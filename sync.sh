#!/bin/bash

set -e

git submodule sync                    
git submodule update --init --remote .
cd build
make -j 4
cd ..
# Only update if tests do not fail (disabled for now)
#./webthree-helpers/scripts/ethtests.sh all --umbrella
git checkout -B develop
git commit -am 'updated submodules'   
git push

