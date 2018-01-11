#!/usr/bin/env python3

"""
JSON-RPC Proxy

This Python script provides HTTP proxy to Unix Socket based JSON-RPC servers.
Check out --help option for more information.

Build with cython:

cython rpcproxy.py --embed
gcc -O3 -I /usr/include/python3.5m -o rpcproxy rpcproxy.c \
-Wl,-Bstatic -lpython3.5m -lz -lexpat -lutil -Wl,-Bdynamic -lpthread -ldl -lm

"""

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from http.server import HTTPServer, BaseHTTPRequestHandler
from os import path
from urllib.parse import urlparse
import errno
import socket
import sys

if sys.platform == 'win32':
    import win32file
    import pywintypes


VERSION = '0.1.0a1'
BUFSIZE = 32
DELIMITER = ord('\n')
INFO = """JSON-RPC Proxy

Version:  {version}
Proxy:    {proxy_url}
Backend:  {backend_url} (connected: {connected})
"""


class BackendError(Exception):
    pass


class UnixSocketConnector(object):
    """Unix Domain Socket connector. Connects to socket lazily."""

    def __init__(self, socket_path):
        self._socket_path = socket_path
        self._socket = None

    @staticmethod
    def _get_error_message(os_error_number):
        if os_error_number == errno.ENOENT:
            return "Unix Domain Socket '{}' does not exist"
        if os_error_number == errno.ECONNREFUSED:
            return "Connection to '{}' refused"
        return "Unknown error when connecting to '{}'"

    def socket(self):
        """Returns connected socket."""
        if self._socket is None:
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(self._socket_path)
                s.settimeout(1)
                # Assign last, to keep it None in case of exception.
                self._socket = s
            except OSError as ex:
                msg = self._get_error_message(ex.errno)
                err = BackendError(msg.format(self._socket_path))
                raise err from ex
        return self._socket

    def close(self):
        if self._socket is not None:
            self._socket.shutdown(socket.SHUT_RDWR)
            self._socket.close()
            self._socket = None

    def is_connected(self):
        return self._socket is not None

    def recv(self, max_length):
        return self.socket().recv(max_length)

    def sendall(self, data):
        try:
            return self.socket().sendall(data)
        except OSError as ex:
            if ex.errno == errno.EPIPE:
                # The connection was terminated by the backend. Try reconnect.
                self.close()
                return self.socket().sendall(data)
            else:
                raise


class NamedPipeConnector(object):
    """Windows named pipe simulating socket."""

    def __init__(self, ipc_path):
        try:
            self.handle = win32file.CreateFile(
                ipc_path, win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0, None, win32file.OPEN_EXISTING, 0, None)
        except pywintypes.error as err:
            raise IOError(err)

    def is_connected(self):
        return True

    def recv(self, max_length):
        (err, data) = win32file.ReadFile(self.handle, max_length)
        if err:
            raise IOError(err)
        return data

    def sendall(self, data):
        return win32file.WriteFile(self.handle, data)

    def close(self):
        self.handle.close()


def get_ipc_connector(ipc_path):
    if sys.platform == 'win32':
        return NamedPipeConnector(ipc_path)
    return UnixSocketConnector(ipc_path)


class HTTPRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path != '/':
            self.send_response(404)
            self.end_headers()
            return

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.addCORS()
        self.end_headers()
        backend_url = 'unix:' + self.server.backend_address
        proxy_url = '{}:{}'.format(self.server.server_name,
                                   self.server.server_port)
        info = INFO.format(version=VERSION, proxy_url=proxy_url,
                           backend_url=backend_url,
                           connected=self.server.conn.is_connected())
        self.wfile.write(info.encode('utf-8'))

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.addCORS()
        self.end_headers()

    def do_POST(self):
        request_length = int(self.headers['Content-Length'])
        request_content = self.rfile.read(request_length)
        # self.log_message("Headers:  {}".format(self.headers))
        # self.log_message("Request:  {}".format(request_content))

        try:
            response_content = self.server.process(request_content)
            # self.log_message("Response: {}".format(response_content))

            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Content-length", len(response_content))
            self.addCORS()
            self.end_headers()
            self.wfile.write(response_content)
        except BackendError as err:
            self.send_response(502)
            error_msg = str(err).encode('utf-8')
            # TODO: Send as JSON-RPC response
            self.send_header("Content-type", "text/plain")
            self.send_header("Content-length", len(error_msg))
            self.end_headers()
            self.wfile.write(error_msg)
            self.log_message("Backend Error: {}".format(err))

        # TODO: Handle other excaptions as error 500.

    def addCORS(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "content-type")


class Proxy(HTTPServer):

    def __init__(self, proxy_url, backend_path):

        proxy_url = urlparse(proxy_url)
        assert proxy_url.scheme == 'http'
        proxy_address = proxy_url.hostname, proxy_url.port

        super(Proxy, self).__init__(proxy_address, HTTPRequestHandler)

        self.backend_address = path.expanduser(backend_path)
        self.conn = get_ipc_connector(self.backend_address)

    def process(self, request):
        self.conn.sendall(request)

        response = b''
        while True:
            r = self.conn.recv(BUFSIZE)
            if not r:
                break
            if r[-1] == DELIMITER:
                response += r[:-1]
                break
            response += r

        return response


def run():
    parser = ArgumentParser(
        description='HTTP Proxy for JSON-RPC servers',
        formatter_class=ArgumentDefaultsHelpFormatter
    )

    if sys.platform == 'win32':
        default_backend_path = r'\\.\pipe\geth.ipc'
        backend_path_help = "Named Pipe of a backend RPC server"
    else:
        default_backend_path = '~/.ethereum/geth.ipc'
        backend_path_help = "Unix Socket of a backend RPC server"

    parser.add_argument('backend_path', nargs='?',
                        default=default_backend_path,
                        help=backend_path_help)
    parser.add_argument('proxy_url', nargs='?',
                        default='http://127.0.0.1:8545',
                        help="URL for this proxy server")
    args = parser.parse_args()
    proxy = Proxy(args.proxy_url, args.backend_path)
    proxy.serve_forever()


if __name__ == '__main__':
    run()
