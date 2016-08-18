#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script to build cpp-ethereum within TravisCI.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

set -e

CMAKE_BUILD_TYPE=$1
TESTS=$2

# There is an implicit assumption here that we HAVE to run from repo root
#
# NOTE - We are disabling OpenCL here to work around currently unexplained
# problems which occur when running "testweb3" unit-tests on Ubuntu.
# The same issue is showing up with the solidity Tests-over-RPC connecting
# to an `eth` node.
#
# Maybe we really need to do something more in our TravisCI configuration which
# ensures that the VM we have can run with this OpenCL support enabled?  But
# this simple approach will do for right now.
#
# See, for example, https://travis-ci.org/ethereum/cpp-ethereum/jobs/152901242
#
# modprobe: ERROR: could not insert 'fglrx': No such device
# Error: Fail to load fglrx kernel module!
# Error! Fail to load fglrx kernel module! Maybe you can switch to root user to load kernel module directly
# modprobe: ERROR: could not insert 'fglrx': No such device
# Error: Fail to load fglrx kernel module!
# Error! Fail to load fglrx kernel module! Maybe you can switch to root user to load kernel module directly
# modprobe: ERROR: could not insert 'fglrx': No such device
# Error: Fail to load fglrx kernel module!
# Error! Fail to load fglrx kernel module! Maybe you can switch to root user to load kernel module directly
# No output has been received in the last 10m0s, this potentially indicates a stalled build or something wrong with the build itself.

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$1 -DTESTS=$2 -DETHASHCL=0
make -j2
