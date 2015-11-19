#!/bin/bash

branch=$(git branch | grep "\*" | cut -c 3-)
for i in webthree-helpers libweb3core libethereum web3.js solidity webthree alethzero mix; do
cd $i
lbranch=$(git branch | grep "\*" | cut -c 3-)
cd ..
if [ "$lbranch" == "$branch" ] ; then
echo Entering $i ...
"$@" "$i"
else
echo "Skipping $i (branch $lbranch is not $branch)".
fi
done
