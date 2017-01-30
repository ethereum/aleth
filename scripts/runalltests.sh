#!/usr/bin/env bash

workdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#Clean the previous build
cd $workdir
rm -r cpp-ethereum
rm -r tests
exec &> $workdir/testlog.txt

#Clonning Repositories
echo "Cloning Repositories"
git clone https://github.com/ethereum/tests.git
git clone --recursive https://github.com/ethereum/cpp-ethereum.git
cd tests
testHead=$(git rev-parse HEAD)
cd ..
cd cpp-ethereum
cppHead=$(git rev-parse HEAD)

#Prepare test results
mkdir build
cd build
echo "Make cpp-ethereum develop:"
cmake ..
make -j8
cd test
echo "Running all tests:"
echo "cpp-ethereum HEAD "$cppHead
echo "tests HEAD "$testHead
./testeth --all --exectimelog

exec &> /dev/null
#cd $workdir
#rm results.zip
#zip results.zip *.txt
date=$(date +%Y-%m-%d)

mail -s "cpp-ethereum test results "$date dimitry@ethereum.org < $workdir/testlog.txt  #-A $workdir/results.zip
mail -s "cpp-ethereum test results "$date chris@ethereum.org < $workdir/testlog.txt #-A $workdir/results.zip
