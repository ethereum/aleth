#!/usr/bin/env python3

import argparse
import subprocess
import sys
import os

from dopple import run_daemon, DEFAULT_PROXY_URL, DEFAULT_BACKEND_PATH

DEFAULT_ALETH_EXEC = '/usr/bin/aleth'

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--aleth-exec', default=DEFAULT_ALETH_EXEC)
parser.add_argument('--rpc', metavar='URL', nargs='?', const=DEFAULT_PROXY_URL, default=False)
parser.add_argument('-d', '--db-path', help=argparse.SUPPRESS)
parser.add_argument('--ipcpath', help=argparse.SUPPRESS)
parser.add_argument('--no-ipc', action='store_true', help=argparse.SUPPRESS)
wrapper_args, aleth_args = parser.parse_known_args()
aleth_exec = wrapper_args.aleth_exec

if not os.path.isfile(aleth_exec):
    print("Wrong path to aleth executable: {}".format(aleth_exec), file=sys.stderr)
    parser.print_usage(sys.stderr)
    exit(1)

ipcpath = DEFAULT_BACKEND_PATH

if wrapper_args.db_path:
    ipcpath = os.path.join(wrapper_args.db_path, 'geth.ipc')
    aleth_args += ['--db-path', wrapper_args.db_path]

if wrapper_args.ipcpath:
    ipcpath = os.path.join(wrapper_args.ipcpath, 'geth.ipc')
    aleth_args += ['--ipcpath', wrapper_args.ipcpath]

if wrapper_args.no_ipc:
    if wrapper_args.rpc:
        print("Option --no-ipc cannot be used with --rpc", file=sys.stderr)
        exit(2)
    else:
        aleth_args += ['--no-ipc']

try:
    if wrapper_args.rpc:
        run_daemon(wrapper_args.rpc, ipcpath)
    p = subprocess.run([aleth_exec] + aleth_args)
    exit(p.returncode)
except KeyboardInterrupt:
    pass
