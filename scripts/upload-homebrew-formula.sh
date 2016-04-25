#!/usr/bin/env bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# Just upload the generated .rb file to homebrew ethereum

echo ">>> Starting the script to upload .rb file to homebrew ethereum"
rm -rf homebrew-ethereum
git clone git@github.com:ethereum/homebrew-ethereum.git
cp webthree-umbrella/build/cpp-ethereum.rb homebrew-ethereum
cd homebrew-ethereum
git add . -u
git commit -m "update cpp-ethereum.rb"
git push origin
cd ..
rm -rf homebrew-ethereum
echo ">>> Succesfully uploaded the .rb file to homebrew ethereum"
