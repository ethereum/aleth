#!/usr/bin/env bash

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
#------------------------------------------------------------------------------
# Bash script implementing release flow for cpp-ethereum for Linux and macOS.
#
# TODO - At the time of writing, we only have ZIPs working.  Need to hook up
# support for Homebrew and PPAs.
#
# The documentation for cpp-ethereum is hosted at:
#
# http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

ZIP_SUFFIX=$1
ZIP_TEMP_DIR=$(pwd)/zip/
TESTS=$2

if [[ "$OSTYPE" == "darwin"* ]]; then
    DLL_EXT=dylib
else
    DLL_EXT=so
fi

mkdir -p $ZIP_TEMP_DIR

# Copy all the cpp-ethereum executables into a temporary directory prior to ZIP creation

cp bench/bench                         $ZIP_TEMP_DIR
cp eth/eth                             $ZIP_TEMP_DIR
cp ethkey/ethkey                       $ZIP_TEMP_DIR
cp ethminer/ethminer                   $ZIP_TEMP_DIR
cp ethvm/ethvm                         $ZIP_TEMP_DIR
cp rlp/rlp                             $ZIP_TEMP_DIR
if [[ "$TESTS" == "On" ]]; then
    cp test/libethereum/test/testeth       $ZIP_TEMP_DIR
    cp test/libweb3core/test/testweb3core  $ZIP_TEMP_DIR
    cp test/webthree/test/testweb3         $ZIP_TEMP_DIR
fi

# Copy all the dynamic libraries into a temporary directory prior to ZIP creation.
# There are a lot of these, and it would be great if we didn't have to worry about them.
# There is work-in-progress to support static-linkage on the UNIX platforms, which
# is most promising on Alpine Linux using musl.  macOS doesn't support statically
# linked binaries (ie. executables which make direct system calls to the kernel.
#
# See https://developer.apple.com/library/mac/qa/qa1118/_index.html.
# See https://github.com/ethereum/webthree-umbrella/issues/495.

cp libdevcore/*.$DLL_EXT               $ZIP_TEMP_DIR
cp libdevcrypto/*.$DLL_EXT             $ZIP_TEMP_DIR
cp libethash/*.$DLL_EXT                $ZIP_TEMP_DIR
cp libethash-cl/*.$DLL_EXT             $ZIP_TEMP_DIR
cp libethashseal/*.$DLL_EXT            $ZIP_TEMP_DIR
cp libethcore/*.$DLL_EXT               $ZIP_TEMP_DIR
cp libethereum/*.$DLL_EXT              $ZIP_TEMP_DIR
cp libevm/*.$DLL_EXT                   $ZIP_TEMP_DIR
cp libevmcore/*.$DLL_EXT               $ZIP_TEMP_DIR
cp libp2p/*.$DLL_EXT                   $ZIP_TEMP_DIR
cp libweb3jsonrpc/*.$DLL_EXT           $ZIP_TEMP_DIR
cp libwebthree/*.$DLL_EXT              $ZIP_TEMP_DIR
cp libwhisper/*.$DLL_EXT               $ZIP_TEMP_DIR
cp utils/libscrypt/*.$DLL_EXT          $ZIP_TEMP_DIR

# For macOS, we also copy the dynamic libraries for our external dependencies.
# When building from source on your own machine, these libraries will be installed
# globally, using Homebrew, but we don't want to rely on that for these ZIPs, so
# we copy these into the ZIP temporary directory too.
#
# TODO - So what happens for Linux and other UNIX distros in this case?
# There will be runtime dependencies on equivalent SO files being present, likely in
# a completely analogous way.   Does that mean that ZIPs are actually useless on such
# distros, because there will be symbol links to global install locations (distro-specific)
# and those files will just be missing on the target machines?

if [[ "$OSTYPE" == "darwin"* ]]; then
    cp /usr/local/opt/cryptopp/lib/libcryptopp.dylib                    $ZIP_TEMP_DIR
    cp /usr/local/opt/gmp/lib/libgmp.10.dylib                           $ZIP_TEMP_DIR
    cp /usr/local/opt/jsoncpp/lib/libjsoncpp.1.dylib                    $ZIP_TEMP_DIR
    cp /usr/local/opt/leveldb/lib/libleveldb.1.dylib                    $ZIP_TEMP_DIR
    cp /usr/local/opt/libjson-rpc-cpp/lib/libjsonrpccpp-common.0.dylib  $ZIP_TEMP_DIR
    cp /usr/local/opt/libjson-rpc-cpp/lib/libjsonrpccpp-client.0.dylib  $ZIP_TEMP_DIR
    cp /usr/local/opt/libjson-rpc-cpp/lib/libjsonrpccpp-server.0.dylib  $ZIP_TEMP_DIR
    cp /usr/local/opt/libmicrohttpd/lib/libmicrohttpd.12.dylib          $ZIP_TEMP_DIR
    cp /usr/local/opt/miniupnpc/lib/libminiupnpc.16.dylib               $ZIP_TEMP_DIR
    cp /usr/local/opt/snappy/lib/libsnappy.1.dylib                      $ZIP_TEMP_DIR
fi

# For macOS, we run a fix-up script which alters all of the symbolic links within
# the executables and dynamic libraries such that the ZIP becomes self-contained, by
# revectoring all the dylib references to be relative to the directory containing the
# application, so that the ZIPs are self-contained, with the only external references
# being for kernel-level dylibs.

if [[ "$OSTYPE" == "darwin"* ]]; then
    python $(pwd)/../homebrew/fix_homebrew_paths_in_standalone_zip.py $ZIP_TEMP_DIR
fi

# And ZIP it all up, with a filename suffix passed in on the command-line.

zip -j $(pwd)/../cpp-ethereum-develop-$ZIP_SUFFIX.zip $ZIP_TEMP_DIR/*
