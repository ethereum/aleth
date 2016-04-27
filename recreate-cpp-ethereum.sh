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
mkdir -p  $outputDirectory/alethzero/
mkdir -p  $outputDirectory/test/

# alethzero intentionally omitted
rsync -r ./dependency_graph/              $outputDirectory/dependency_graph/
rsync -r ./doc/                           $outputDirectory/doc/
rsync -r ./docker/                        $outputDirectory/docker/
rsync -r ./libethereum/ConfigInfo.h.in    $outputDirectory/ConfigInfo.h.in
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
rsync -r ./libethereum/libnatspec/        $outputDirectory/libnatspec/
rsync -r ./libethereum/libtestutils/      $outputDirectory/libtestutils/
rsync -r ./libethereum/test/deprecated/             $outputDirectory/test/deprecated/
rsync -r ./libethereum/test/external-dependencies/  $outputDirectory/test/external-dependencies/
rsync -r ./libethereum/test/fuzzTesting/            $outputDirectory/test/fuzzTesting/
rsync -r ./libethereum/test/libethcore/             $outputDirectory/test/libethcore/
rsync -r ./libethereum/test/libethereum/            $outputDirectory/test/libethereum/
rsync -r ./libethereum/test/libevm/                 $outputDirectory/test/libevm/
rsync -r ./libethereum/test/libnatspec/             $outputDirectory/test/libnatspec/
rsync -r ./libethereum/test/libweb3core/            $outputDirectory/test/libweb3core/
rsync -r ./libethereum/test/BlockChainHelper.cpp    $outputDirectory/test/BlockChainHelper.cpp
rsync -r ./libethereum/test/BlockChainHelper.h      $outputDirectory/test/BlockChainHelper.h
rsync -r ./libethereum/test/boostTest.cpp           $outputDirectory/test/boostTest.cpp
rsync -r ./libethereum/test/JSON_test.sol           $outputDirectory/test/JSON_test.sol
rsync -r ./libethereum/test/JsonSpiritHeaders.h     $outputDirectory/test/JsonSpiritHeaders.h
rsync -r ./libethereum/test/Stats.cpp               $outputDirectory/test/Stats.cpp
rsync -r ./libethereum/test/Stats.h                 $outputDirectory/test/Stats.h
rsync -r ./libethereum/test/TestHelper.cpp          $outputDirectory/test/TestHelper.cpp
rsync -r ./libethereum/test/TestHelper.h            $outputDirectory/test/TestHelper.h
rsync -r ./libethereum/test/TestUtils.cpp           $outputDirectory/test/TestUtils.cpp
rsync -r ./libethereum/test/TestUtils.h             $outputDirectory/test/TestUtils.h
rsync -r ./libweb3core/bench/             $outputDirectory/bench/
rsync -r ./libweb3core/libdevcore/        $outputDirectory/libdevcore/
rsync -r ./libweb3core/libdevcrypto/      $outputDirectory/libdevcrypto/
rsync -r ./libweb3core/libp2p/            $outputDirectory/libp2p/
rsync -r ./libweb3core/rlp/               $outputDirectory/rlp/
rsync -r ./libweb3core/test/libdevcore/   $outputDirectory/test/libdevcore/
rsync -r ./libweb3core/test/libdevcrypto/ $outputDirectory/test/libdevcrypto/
rsync -r ./libweb3core/test/libp2p/       $outputDirectory/test/libp2p/
rsync -r ./libweb3core/test/memorydb.cpp  $outputDirectory/test/memorydb.cpp
rsync -r ./libweb3core/test/overlaydb.cpp $outputDirectory/test/overlaydb.cpp
rsync -r ./libweb3core/test/test.cpp      $outputDirectory/test/test.cpp
rsync -r ./libweb3core/test/test.h        $outputDirectory/test/test.h
# mix intentionally omitted
# res intentionally omitted
# solidity intentionally omitted
# web3.js intentionally omitted
rsync -r ./webthree/eth/                  $outputDirectory/eth/
rsync -r ./webthree/libweb3jsonrpc/       $outputDirectory/libweb3jsonrpc/
rsync -r ./webthree/libwebthree/          $outputDirectory/libwebthree/
rsync -r ./webthree/libwhisper/           $outputDirectory/libwhisper/
rsync -r ./webthree/test/ethrpctest/      $outputDirectory/test/ethrpctest/
rsync -r ./webthree/test/libweb3jsonrpc/  $outputDirectory/test/libweb3jsonrpc/
rsync -r ./webthree/test/libwhisper/      $outputDirectory/test/libwhisper/
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
rsync -r ./astylerc                       $outputDirectory/astylerc
rsync -r ./CodingStandards.txt            $outputDirectory/CodingStandards.txt
rsync -r ./CONTRIBUTING.md                $outputDirectory/CONTRIBUTING.md
rsync -r ./diffscript.sh                  $outputDirectory/diffscript.sh
rsync -r ./foreach.sh                     $outputDirectory/foreach.sh
rsync -r ./getcoverage.sh                 $outputDirectory/getcoverage.sh
rsync -r ./getdev.sh                      $outputDirectory/getdev.sh
rsync -r ./GPLV3_LICENSE                  $outputDirectory/GPLV3_LICENSE
rsync -r ./LICENSE                        $outputDirectory/LICENSE
rsync -r ./nameeach.sh                    $outputDirectory/nameeach.sh
rsync -r ./new.sh                         $outputDirectory/new.sh
rsync -r ./push.sh                        $outputDirectory/push.sh
rsync -r ./sanitizer-blacklist.txt        $outputDirectory/sanitizer-blacklist.txt
rsync -r ./sync.sh                        $outputDirectory/sync.sh

# These CMakeLists.txt were manually synthesized by Bob.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/CMakeLists.txt > $outputDirectory/CMakeLists.txt
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/test/CMakeLists.txt > $outputDirectory/test/CMakeLists.txt

# This README.md is going to need to be manually updated to reflect the new repo organizational reality,
# which will need to be some kind of merger of the README.md content from cpp-ethereum and from webthree-umbrella.
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/README.md > $outputDirectory/README.md

# TODO - evmjit submodule will need "hooking up", for now we'll just git clone it into a local directory to get
# the content we need for testing.
git clone https://github.com/ethereum/evmjit $outputDirectory/evmjit

# TODO - Need to upstream my edits from https://github.com/bobsummerwill/cpp-ethereum/commits/merge_repos.
# TODO - Move Contributing and coding standards to http://ethdocs.org
# TODO - Where will qtcreator-style go?   Ditto for res folder.
