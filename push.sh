#!/bin/bash

cd $1
git pull
git push git@github.com:ethereum/$1
cd ..
./sync.sh

