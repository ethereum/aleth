#!/usr/bin/env python3

import argparse
import subprocess
import sys
import os

from jsonrpcproxy import run_daemon, DEFAULT_PROXY_URL

DEFAULT_ALETH_EXEC = '/usr/bin/aleth'

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--aleth-exec', default=DEFAULT_ALETH_EXEC)
parser.add_argument('--rpc', nargs='?', const=DEFAULT_PROXY_URL, default=False)
wrapper_args, aleth_args = parser.parse_known_args()
aleth_exec = wrapper_args.aleth_exec

if not os.path.isfile(aleth_exec):
    print("Wrong path to aleth executable: {}".format(aleth_exec), file=sys.stderr)
    parser.print_usage(sys.stderr)
    exit(1)

try:
    if wrapper_args.rpc:
        run_daemon(wrapper_args.rpc)
    p = subprocess.run([aleth_exec] + aleth_args)
    exit(p.returncode)
except KeyboardInterrupt:
    pass
