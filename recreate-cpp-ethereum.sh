#!/usr/bin/env bash
#
# Script to recreate the cpp-ethereum repository by copying directories from webthree-umbrella.
#
# This is for one step of the "Dry run for repository reorganization" which is to get the content
# correct, automated and tested, before pulling the trigger on the real move, which will need to
# preserve history.
#
# See https://github.com/ethereum/webthree-umbrella/issues/453

outputDirectory=../cpp-ethereum

#rm    -rf $outputDirectory
mkdir -p  $outputDirectory/scripts/
mkdir -p  $outputDirectory/test/libethereum/
mkdir -p  $outputDirectory/test/libweb3core/
mkdir -p  $outputDirectory/test/webthree/

rsync -r ./doc/                           $outputDirectory/doc/
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
# intentionally left /libethereum root files behind: (CMakeLists.txt, fetch_umbrella_build_and_test.sh, install_dependencies.sh, LICENSE, README.md)
rsync -r ./libweb3core/bench/             $outputDirectory/bench/
rsync -r ./libweb3core/libdevcore/        $outputDirectory/libdevcore/
rsync -r ./libweb3core/libdevcrypto/      $outputDirectory/libdevcrypto/
rsync -r ./libweb3core/libp2p/            $outputDirectory/libp2p/
rsync -r ./libweb3core/rlp/               $outputDirectory/rlp/
rsync -r ./libweb3core/test/              $outputDirectory/test/libweb3core/test/
# intentionally left /libweb3core root files behind: (CMakeLists.txt, LICENSE, README.md)
rsync -r ./webthree/eth/                  $outputDirectory/eth/
rsync -r ./webthree/libweb3jsonrpc/       $outputDirectory/libweb3jsonrpc/
rsync -r ./webthree/libwebthree/          $outputDirectory/libwebthree/
rsync -r ./webthree/libwhisper/           $outputDirectory/libwhisper/
rsync -r ./webthree/test/                 $outputDirectory/test/webthree/test/
# intentionally left /webthree root files behind: (CMakeLists.txt, LICENSE, README.md)
rsync -r ./webthree-helpers/cmake/        $outputDirectory/cmake/
rsync -r ./webthree-helpers/homebrew/     $outputDirectory/homebrew/
rsync -r ./webthree-helpers/scripts/ppabuild.sh                $outputDirectory/scripts/ppabuild.sh
rsync -r ./webthree-helpers/scripts/upload-homebrew-formula.sh $outputDirectory/scripts/upload-homebrew-formula.sh
rsync -r ./webthree-helpers/utils/        $outputDirectory/utils/
# intentionally left /webthree-helpers root files behind: (LICENSE, README.md)

# .gitignore intentionally omitted.
# .gitmodules intentionally omitted.
# CMakeLists.txt intentionally omitted.
rsync -r ./CodingStandards.txt            $outputDirectory/CodingStandards.txt
rsync -r ./CONTRIBUTING.md                $outputDirectory/CONTRIBUTING.md
rsync -r ./LICENSE                        $outputDirectory/LICENSE
# README.md intentionally omitted.
rsync -r ./sanitizer-blacklist.txt        $outputDirectory/sanitizer-blacklist.txt

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
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/install_deps.bat > $outputDirectory/scripts/install_deps.bat
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/install_deps.sh > $outputDirectory/scripts/install_deps.sh
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/release.bat > $outputDirectory/scripts/release.bat
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/release.sh > $outputDirectory/scripts/release.sh
curl https://raw.githubusercontent.com/bobsummerwill/cpp-ethereum/merge_repos/scripts/tests.bat > $outputDirectory/scripts/tests.bat
