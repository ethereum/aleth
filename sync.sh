#!/bin/bash

set -e

git submodule sync                    
git submodule update --init --remote .
mkdir -p build
cd build
cmake ..
make -j 4
cd ..
# Only update if tests do not fail (disabled for now)
#./webthree-helpers/scripts/ethtests.sh all --umbrella
git checkout -B develop
git commit -am 'updated submodules'   
echo "You can now push the commit."
# We do not push as part of the script because
# it complicates user access issues. On Jenkins,
# this is done by a post-build publish step.
#git push

