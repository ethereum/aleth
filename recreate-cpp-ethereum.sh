#!/usr/bin/env bash
#
# Script to recreate the cpp-ethereum repository by copying directories from webthree-umbrella.
#
# This is for one step of the "Dry run for repository reorganization" which is to get the content
# correct, automated and tested, before pulling the trigger on the real move, which will need to
# preserve history.
#
# See https://github.com/ethereum/webthree-umbrella/issues/453

outputDirectory=../cpp-ethereum-recreated

rm    -rf $outputDirectory
mkdir -p  $outputDirectory
mkdir -p  $outputDirectory/test/libethereum/
mkdir -p  $outputDirectory/test/libweb3core/
mkdir -p  $outputDirectory/test/webthree/

# alethzero intentionally omitted
rsync -r ./dependency_graph/              $outputDirectory/dependency_graph/
rsync -r ./doc/                           $outputDirectory/doc/
rsync -r ./docker/                        $outputDirectory/docker/
rsync -r ./libethereum/ethkey/            $outputDirectory/ethkey/
rsync -r ./libethereum/ethminer/          $outputDirectory/ethminer/
rsync -r ./libethereum/ethvm/             $outputDirectory/ethvm/
rsync -r ./libethereum/libethash/         $outputDirectory/libethash/
rsync -r ./libethereum/libethash-cl/      $outputDirectory/libethash-cl/
rsync -r ./libethereum/libethashseal/     $outputDirectory/libethashseal/
rsync -r ./libethereum/libethcore/        $outputDirectory/libethcore/
rsync -r ./libethereum/libethereum/       $outputDirectory/libethereum/
rsync -r ./libethereum/libevm/            $outputDirectory/libevm/
rsync -r ./libethereum/libevmcore/        $outputDirectory/libevmcore/
rsync -r ./libethereum/test/              $outputDirectory/test/libethereum/test/
rsync -r ./libweb3core/bench/             $outputDirectory/bench/
rsync -r ./libweb3core/libdevcore/        $outputDirectory/libdevcore/
rsync -r ./libweb3core/libdevcrypto/      $outputDirectory/libdevcrypto/
rsync -r ./libweb3core/libp2p/            $outputDirectory/libp2p/
rsync -r ./libweb3core/rlp/               $outputDirectory/rlp/
rsync -r ./libweb3core/test/              $outputDirectory/test/libweb3core/test/
# mix intentionally omitted
# res intentionally omitted
# solidity intentionally omitted
# web3.js intentionally omitted
rsync -r ./webthree/eth/                  $outputDirectory/eth/
rsync -r ./webthree/libweb3jsonrpc/       $outputDirectory/libweb3jsonrpc/
rsync -r ./webthree/libwebthree/          $outputDirectory/libwebthree/
rsync -r ./webthree/libwhisper/           $outputDirectory/libwhisper/
rsync -r ./webthree/test/                 $outputDirectory/test/webthree/test/
rsync -r ./webthree-helpers/cmake/        $outputDirectory/cmake/
rsync -r ./webthree-helpers/extdep/       $outputDirectory/extdep/
rsync -r ./webthree-helpers/homebrew/     $outputDirectory/homebrew/
rsync -r ./webthree-helpers/js/           $outputDirectory/js/
rsync -r ./webthree-helpers/scripts/      $outputDirectory/scripts/
rsync -r ./webthree-helpers/templates/    $outputDirectory/templates/
rsync -r ./webthree-helpers/utils/        $outputDirectory/utils/
# intentionally left /webthree-helpers root files behind: (LICENSE, new.sh, README.md)
# TODO /webthree-helpers/cmake has (LICENSE, README.md), but why?
# TODO /webthree-helpers/homebrew has (LICENSE, README.md), but why?
# Tried unsuccessfully to delete homebrew ones.   Needed in some release flow?

# Loose files in the root directory of webthree-umbrella.
# TODO - Move all these loose scripts in the root into /scripts
# CMakeLists.txt intentionally omitted.
rsync -r ./CodingStandards.txt            $outputDirectory/CodingStandards.txt
rsync -r ./CONTRIBUTING.md                $outputDirectory/CONTRIBUTING.md
rsync -r ./LICENSE                        $outputDirectory/LICENSE
# qtcreator-style intentionally omitted.
# README.md intentionally omitted.
rsync -r ./sanitizer-blacklist.txt        $outputDirectory/sanitizer-blacklist.txt
rsync -r ./sync.sh                        $outputDirectory/sync.sh

# These files cannot be upstreamed, but instead need to be manually maintained and then dropped into 'cpp-ethereum' when we merge.
# These CMakeLists.txt were manually synthesized by Bob.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/cmake/EthOptions.cmake > $outputDirectory/cmake/EthOptions.cmake
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/CMakeLists.txt > $outputDirectory/CMakeLists.txt
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/README.md > $outputDirectory/README.md

# These files could be upstreamed, but it isn't worth doing so, because they can only be used after the repo reorganization.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.gitignore > $outputDirectory/.gitignore
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.gitmodules > $outputDirectory/.gitmodules
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/.travis.yml > $outputDirectory/.travis.yml
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/appveyor.yml > $outputDirectory/appveyor.yml
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/install_deps.bat > $outputDirectory/install_deps.bat
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/install_deps.sh > $outputDirectory/install_deps.sh

# TODO - evmjit and deps submodules will need "hooking up", for now we'll just git clone them into local directories to get
# the content we need for testing.
#git clone https://github.com/ethereum/cpp_dependencies $outputDirectory/deps
#git clone https://github.com/ethereum/evmjit $outputDirectory/evmjit

# TODO - Move Contributing and coding standards to http://ethdocs.org
