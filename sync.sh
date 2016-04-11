#!/bin/bash

set -e

git submodule sync                    
git submodule update --init --remote .
cd build
make -j 4
cd ..
./webthree-helpers/scripts/ethtests.sh libethereum --umbrella
git commit -am 'updated submodules'   
git push

