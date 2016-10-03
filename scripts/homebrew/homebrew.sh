#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for the Homebrew publishing process for cpp-ethereum.
#
# Homebrew is the self-described "Missing package manager for OS X".
# See http://brew.sh/.  We publish binaries ("bottles" to Homebrew) for:
#
#     - OS X Mavericks (10.9)
#     - OS X Yosemite (10.10)
#     - OS X El Capitan (10.11)
#     - macOS Sierra (10.12)
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

# There is an implicit assumption that this script is running within the build
# directory of cpp-ethereum.

VERSION=1 # eg 1.0rc2
NUMBER=1 # jenkins build number

# Detect whether we are running on a Yosemite or El Capitan machine, and generate
# an appropriately named ZIP file for the Homebrew receipt to point at.
if echo `sw_vers` | grep "10.9"; then
    OSX_VERSION=mavericks
elif echo `sw_vers` | grep "10.10"; then
    OSX_VERSION=yosemite
elif echo `sw_vers` | grep "10.11"; then
    OSX_VERSION=el_capitan
elif echo `sw_vers` | grep "10.12"; then
    OSX_VERSION=sierra
else
    echo Unsupported macOS version.  We only support Yosemite, El Capitan and Sierra
    exit 1
fi

echo "Homebrew publishing for ${OSX_VERSION}"

while [ "$1" != "" ]; do
    case $1 in
        --version )
            shift
            VERSION=$1 
            ;;
        --number )
            shift
            NUMBER=$1
            ;;
    esac
    shift
done

# prepare template directory
rm -rf cpp-ethereum
mkdir cpp-ethereum
mkdir cpp-ethereum/$VERSION
cp ../scripts/homebrew/homebrew.mxcl.cpp-ethereum.plist ../scripts/homebrew/INSTALL_RECEIPT.json ../LICENSE cpp-ethereum/$VERSION

# run 'make install', which copies the final executables and
# dylibs into a temporary directory with correct final structure.
cmake .. -DCMAKE_INSTALL_PREFIX=install -DTESTS=Off
mkdir -p install
make install
cp -rf install/* cpp-ethereum/$VERSION

# tar everything
NAME="cpp-ethereum-${VERSION}.${OSX_VERSION}.bottle.${NUMBER}.tar.gz"
tar -zcvf $NAME cpp-ethereum

# get variables
HASH=`git rev-parse HEAD`
SIGNATURE=`openssl sha1 ${NAME} | cut -d " " -f 2`

echo "HASH = ${HASH}"
echo "SIGNATURE = ${SIGNATURE}"

# Pull the current cpp-ethereum.rb file from Github.  We used to use a template file.
curl https://raw.githubusercontent.com/bobsummerwill/homebrew-ethereum/fix_bottles/cpp-ethereum.rb > cpp-ethereum.rb.in

# prepare receipt
if [ ${OSX_VERSION} == yosemite ]; then
    sed -e s/revision\ \=\>\ \'[[:xdigit:]][[:xdigit:]]*\'/revision\ \=\>\ \'${HASH}\'/g \
        -e s/version\ \'.*\'/version\ \'${VERSION}\'/g \
        -e s/sha1\ \'[[:xdigit:]][[:xdigit:]]*\'\ \=\>\ \:\yosemite/sha1\ \'${SIGNATURE}\'\ \=\>\ \:yosemite/g \
        -e s/revision[[:space:]][[:digit:]][[:digit:]]*/revision\ ${NUMBER}/g < cpp-ethereum.rb.in > "cpp-ethereum.rb"
else
    sed -e s/revision\ \=\>\ \'[[:xdigit:]][[:xdigit:]]*\'/revision\ \=\>\ \'${HASH}\'/g \
        -e s/version\ \'.*\'/version\ \'${VERSION}\'/g \
        -e s/sha1\ \'[[:xdigit:]][[:xdigit:]]*\'\ \=\>\ \:\el\_capitan/sha1\ \'${SIGNATURE}\'\ \=\>\ \:el\_capitan/g \
        -e s/revision[[:space:]][[:digit:]][[:digit:]]*/revision\ ${NUMBER}/g < cpp-ethereum.rb.in > "cpp-ethereum.rb"
fi

# And then upload formula to the tap on Github
echo ">>> Starting the script to upload .rb file to homebrew ethereum"
rm -rf homebrew-ethereum
# TODO - Correct this to point to ethereum/homebrew-ethereum prior to merge.
git clone git@github.com:bobsummerwill/homebrew-ethereum.git
cp cpp-ethereum.rb homebrew-ethereum
cd homebrew-ethereum
git add . -u
git commit -m "update cpp-ethereum.rb"
git push origin
cd ..
rm -rf homebrew-ethereum
echo ">>> Succesfully uploaded the .rb file to homebrew ethereum"
