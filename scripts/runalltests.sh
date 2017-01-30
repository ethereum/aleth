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
exec 2> $workdir/testerror.txt
timestart=$(date +%s.%N)
./testeth --all --exectimelog
timeend=$(date +%s.%N)

#Prepare Header of the report
exec &> $workdir/testreport.txt
exectime=$(echo "$timeend - $timestart" | bc)
echo "REPORT"
echo "Test execution time: $exectime s"
cat $workdir/testerror.txt
cat $workdir/testreport.txt | cat - $workdir/testlog.txt > temp && mv temp $workdir/testlog.txt

#Send Mails
date=$(date +%Y-%m-%d)
mail -s "cpp-ethereum test results "$date dimitry@ethereum.org < $workdir/testlog.txt  #-A $workdir/results.zip
mail -s "cpp-ethereum test results "$date chris@ethereum.org < $workdir/testlog.txt #-A $workdir/results.zip
