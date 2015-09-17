#/bin/bash

# this script should be run from build directory

VERSION=1 # eg 1.0rc2
NUMBER=1 # jenkins build number

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
cp homebrew.mxcl.cpp-ethereum.plist INSTALL_RECEIPT.json LICENSE README.md cpp-ethereum/$VERSION

# build umbrella project and move install directory to destination
mv install/* cpp-ethereum/$VERSION

#tar everything
NAME="cpp-ethereum-${VERSION}.yosemite.bottle.${NUMBER}.tar.gz"
tar -zcvf $NAME cpp-ethereum

# get variables
HASH=`git rev-parse HEAD`
SIGNATURE=`openssl sha1 ethereum.rb | cut -d " " -f 2`

# prepare receipt
sed -e s/CFG_NUMBER/${NUMBER}/g \
    -e s/CFG_SIGNATURE/${SIGNATURE}/g \
    -e s/CFG_HASH/${HASH}/g \
    -e s/CFG_NUMBER/${NUMBER}/g < cpp-ethereum.rb.in > cpp-ethereum.rb

