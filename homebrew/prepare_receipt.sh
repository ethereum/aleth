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
p="../webthree-helpers/homebrew/"
cp ${p}homebrew.mxcl.cpp-ethereum.plist ${p}INSTALL_RECEIPT.json ${p}LICENSE ${p}README.md cpp-ethereum/$VERSION

# build umbrella project and move install directory to destination
mkdir -p install
cp -rf install/* cpp-ethereum/$VERSION

#tar everything
NAME="cpp-ethereum-${VERSION}.yosemite.bottle.${NUMBER}.tar.gz"
tar -zcvf $NAME cpp-ethereum

# get variables
HASH=`git rev-parse HEAD`
SIGNATURE=`openssl sha1 ${NAME} | cut -d " " -f 2`

# prepare receipt
sed -e s/CFG_NUMBER/${NUMBER}/g \
    -e s/CFG_SIGNATURE/${SIGNATURE}/g \
    -e s/CFG_HASH/${HASH}/g \
    -e s/CFG_VERSION/${VERSION}/g < ${p}cpp-ethereum.rb.in > "cpp-ethereum.rb"

