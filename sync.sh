#!/bin/bash

git submodule sync                    
git submodule update --init --remote .
git commit -am 'updated submodules'   
git push

