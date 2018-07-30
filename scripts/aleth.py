#!/usr/bin/env python3

import argparse
import subprocess

from jsonrpcproxy import run_daemon

DEFAULT_ALETH_EXEC = '/usr/bin/aleth'

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--aleth-exec', default=DEFAULT_ALETH_EXEC)
parser.add_argument('--rpc', action='store_true')
wrapper_args, aleth_args = parser.parse_known_args()

try:
    if wrapper_args.rpc:
        run_daemon()
    p = subprocess.run([wrapper_args.aleth_exec] + aleth_args)
    exit(p.returncode)
except KeyboardInterrupt:
    pass
