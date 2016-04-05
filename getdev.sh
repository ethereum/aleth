#!/bin/bash

for i in libweb3core libethereum webthree alethzero; do
cd $i
git checkout develop
cd ..
done
