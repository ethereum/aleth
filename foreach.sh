#!/bin/bash

for i in libweb3core libethereum webthree alethzero; do
cd $i
"$@"
cd ..
done
