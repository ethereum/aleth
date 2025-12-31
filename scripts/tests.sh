#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script to execute the cpp-ethereum tests.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2019 Aleth Authors.
# Licensed under the GNU General Public License, Version 3.
#------------------------------------------------------------------------------

set -e

TESTS=$1

# There is an implicit assumption here that we HAVE to run from build directory.
BUILD_ROOT=$(pwd)

if [[ "$TESTS" == "On" ]]; then
    # Run the tests for the Interpreter
    $BUILD_ROOT/test/testeth -- --testpath $BUILD_ROOT/../test/jsontests

    # Fill some state tests and make sure the result passes linting
    echo "Running testeth filler tests..."
    $BUILD_ROOT/test/testeth -t GeneralStateTests/stExample -- --filltests --testpath $BUILD_ROOT/../test/jsontests
    $BUILD_ROOT/test/testeth -t 'TransitionTests/bcEIP158ToByzantium' -- --filltests --singletest  ByzantiumTransition --testpath $BUILD_ROOT/../test/jsontests
    cd $BUILD_ROOT/../test/jsontests

    npm init -y
    npm install jsonschema
    echo -e "$(find GeneralStateTests/stExample -name '*.json')" | node JSONSchema/validate.js JSONSchema/st-schema.json
    echo -e "BlockchainTests/TransitionTests/bcEIP158ToByzantium/ByzantiumTransition.json" | node JSONSchema/validate.js JSONSchema/bc-schema.json

    # Run the tests for the JIT (but only for Ubuntu, not macOS)
    # The whole automation process is too slow for macOS, and we don't have
    # enough time to build LLVM, build EVMJIT and run the tests twice within
    # the 48 minute absolute maximum run time for TravisCI.

    # Disabled until EVMJIT will catch up with Metropolis features.
    # if [[ "$OSTYPE" != "darwin"* ]]; then
    #     $BUILD_ROOT/test/testeth -t "VMTests*,StateTests*" -- --vm jit --testpath $BUILD_ROOT/../test/jsontests
    # fi

fi
