#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script to execute the cpp-ethereum tests.
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

TESTS=$1

# There is an implicit assumption here that we HAVE to run from build directory.
BUILD_ROOT=$(pwd)

# TODO Add handling for error codes
if [[ "$TESTS" == "On" ]]; then

    cd ../..
    git clone https://github.com/ethereum/tests.git
    export ETHEREUM_TEST_PATH=$(pwd)/tests/
    cd cpp-ethereum/build

    $BUILD_ROOT/test/libethereum/test/testeth
    $BUILD_ROOT/test/libweb3core/test/testweb3core

    # Disable testweb3 tests for Ubuntu temporarily.  It looks like they are
    # hanging.  Maybe some OpenCL configuration issue, or maybe something else?
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

    if [[ "$OSTYPE" == "darwin"* ]]; then
        $BUILD_ROOT/test/webthree/test/testweb3
    fi

fi
