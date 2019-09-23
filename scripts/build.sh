#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script to build cpp-ethereum within TravisCI.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2019 Aleth Authors.
# Licensed under the GNU General Public License, Version 3.
#------------------------------------------------------------------------------

set -e -x

# There is an implicit assumption here that we HAVE to run from repo root

mkdir -p build
cd build
if [ $(uname -s) == "Linux" ]; then
#	Disabling while llvm-3.9 package is broken
#    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON -DEVMJIT=ON -DLLVM_DIR=/usr/lib/llvm-3.9/lib/cmake/llvm
    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON
else
    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON
fi

make -j2
