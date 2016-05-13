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

if [[ "$OSTYPE" == "darwin"* ]]; then
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

elif [[ "$OSTYPE" == "linux-gnu" ]]; then

    sudo apt-add-repository ppa:george-edison55/cmake-3.x

    sudo add-apt-repository "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.7 main"
    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -

    sudo add-apt-repository -y ppa:ethereum/ethereum-qt
    sudo add-apt-repository -y ppa:ethereum/ethereum
    sudo add-apt-repository -y ppa:ethereum/ethereum-dev

    sudo apt-get -y update
    sudo apt-get -y upgrade

    sudo apt-get -y install \
        build-essential \
        git \
        cmake \
        language-pack-en-base \
        libboost-all-dev \
        libgmp-dev \
        libleveldb-dev \
        libminiupnpc-dev \
        libreadline-dev \
        libncurses5-dev \
        libcurl4-openssl-dev \
        libcryptopp-dev \
        libmicrohttpd-dev \
        libjsoncpp-dev \
        libjson-rpc-cpp-dev \
        libargtable2-dev \
        libedit-dev \
        llvm-3.7-dev \
        mesa-common-dev \
        ocl-icd-libopencl1 \
        opencl-headers \
        libgoogle-perftools-dev \
        qml-module-qtquick-controls \
        qml-module-qtwebengine \
        qtbase5-dev \
        qt5-default \
        qtdeclarative5-dev \
        libqt5webkit5-dev \
        libqt5webengine5-dev \
        ocl-icd-dev \
        libv8-dev \
        libz-dev

        sudo dpkg-reconfigure locales
fi
