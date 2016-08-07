#!/usr/bin/env bash
#
# History-preserving script to recreate the cpp-ethereum repository.
#
# See also 'recreate-cpp-ethereum.sh' which was the basis for getting
# the content correct.   This script attempts to replicate that
# content, but also to migrate all the relevant history.
#
# Needs just a little more tweaking on the commands for specific
# files to be treated correctly.
#
# Main TODO remaining is that merging all four repos together
# like this results in redundant commits during the period of
# time where all of these repos had common history.   We are
# ending up with up to 4 copies of some of the commits during
# that period.  We need to address that, but everything else
# can be lined up anyway.
#
# See https://github.com/ethereum/webthree-umbrella/issues/453


cd ..
rm -rf cpp-ethereum-with-history
git clone https://github.com/bobsummerwill/cpp-ethereum cpp-ethereum-with-history
cd cpp-ethereum-with-history

git remote rm origin
git remote add origin https://github.com/ethereum/webthree-umbrella
git pull --no-edit --strategy-option theirs origin develop
git rm .gitmodules
git add CMakeLists.txt
git add CodingStandards.txt
git rm recreate-cpp-ethereum.sh
git rm recreate-cpp-ethereum-with-history.sh
git rm --cached libethereum
git rm --cached libweb3core
git rm --cached webthree
git rm --cached webthree-helpers
rm -rf libweb3core
rm -rf webthree
rm -rf webthree-helpers
rm recreate-cpp-ethereum.sh
rm recreate-cpp-ethereum-with-history.sh
rm .gitmodules
git commit -m "webthree-umbrella merge"
#git push

git remote rm origin
git remote add origin https://github.com/bobsummerwill/libethereum
git pull --no-edit --strategy-option theirs origin reorg
git rm --cached fetch_umbrella_build_and_test.sh
git rm install_dependencies.sh
git commit -m "libethereum merge"
#git push

git remote rm origin
git remote add origin https://github.com/bobsummerwill/libweb3core
git pull --no-edit --strategy-option theirs origin reorg
git commit -m "libweb3core merge"
#git push

git remote rm origin
git remote add origin https://github.com/bobsummerwill/webthree
git pull --no-edit --strategy-option theirs origin reorg
git commit -m "webthree merge"
#git push

git remote rm origin
git remote add origin https://github.com/bobsummerwill/webthree-helpers
git pull --no-edit --strategy-option theirs origin reorg
git commit -m "webthree-helpers merge"
#git push

# These files cannot be upstreamed, but instead need to be manually maintained and then dropped into 'cpp-ethereum' when we merge.
# These CMakeLists.txt were manually synthesized by Bob.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/cmake/EthOptions.cmake > ./cmake/EthOptions.cmake
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/CMakeLists.txt > ./CMakeLists.txt
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/README.md > ./README.md

# These files could be upstreamed, but it isn't worth doing so, because they can only be used after the repo reorganization.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.gitignore > ./.gitignore
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.gitmodules > ./.gitmodules
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.travis.yml > ./.travis.yml
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/appveyor.yml > ./appveyor.yml
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/install_deps.bat > ./scripts/install_deps.bat
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/install_deps.sh > ./scripts/install_deps.sh
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/release.bat > ./scripts/release.bat
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/release.sh > ./scripts/release.sh
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/tests.bat > ./scripts/tests.bat

# This should come from somewhere with its history.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/sanitizer-blacklist.txt > ./sanitizer-blacklist.txt
curl https://raw.githubusercontent.com/ethereum/webthree-umbrella/develop/doc/Doxyfile > ./doc/Doxyfile

# Tweak tests CMake files
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/test/libethereum/test/CMakeLists.txt > ./test/libethereum/test/CMakeLists.txt
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/test/libweb3core/test/CMakeLists.txt > ./test/libweb3core/test/CMakeLists.txt
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/test/webthree/test/CMakeLists.txt > ./test/webthree/test/CMakeLists.txt

git add CMakeLists.txt
git add .gitignore
git add .travis.yml
git add appveyor.yml
git add doc/Doxyfile
git add cmake/EthOptions.cmake
git add scripts/install_deps.bat
git add scripts/install_deps.sh
git add scripts/release.bat
git add scripts/release.sh
git add scripts/tests.bat
git add test/libethereum/test/CMakeLists.txt
git add test/libweb3core/test/CMakeLists.txt
git add test/webthree/test/CMakeLists.txt
git commit -m "new files"

git submodule add https://github.com/ethereum/cpp-dependencies deps
git submodule add https://github.com/ethereum/evmjit evmjit
git commit -m "submodules"

#exit 0

git remote rm origin
git remote add origin https://github.com/bobsummerwill/cpp-ethereum-with-history
git push --set-upstream origin develop
git push


cd ..
cd webthree-umbrella
