#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for installing pre-requisite packages for cpp-ethereum.
#
# The documentation for cpp-ethereum is hosted at:
#
# http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    # TODO
elif [[ "$OSTYPE" == "darwin"* ]]; then
    brew update
    brew upgrade
    brew install boost
    brew install cmake
    brew install cryptopp
    brew install miniupnpc
    brew install leveldb
    brew install gmp
    brew install jsoncpp
    brew install libmicrohttpd
    brew install libjson-rpc-cpp
    brew install homebrew/versions/llvm37
fi
