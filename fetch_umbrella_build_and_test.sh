#!/usr/bin/env bash

set -ev

cd ..
git clone --depth 3 -b develop https://github.com/ethereum/tests.git
export ETHEREUM_TEST_PATH=$(pwd)/tests/
git clone --recursive -b develop https://github.com/ethereum/webthree-umbrella.git
cd webthree-umbrella
rm -rf libethereum
mv ../libethereum .
mkdir build
cd build
cmake .. -DGUI=0 -DCMAKE_BUILD_TYPE=$TRAVIS_BUILD_TYPE
make -j 2 eth ethvm testeth
./libethereum/test/testeth
