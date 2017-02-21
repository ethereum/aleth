#!/usr/bin/env bash

workdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#Clean the previous build
cd $workdir
rm -r cpp-ethereum
rm -r tests
exec &> $workdir/buildlog.txt

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
echo "cpp-ethereum repository at commit $cppHead"
echo "tests repository at commit $testHead"
exec 2> $workdir/testlog.txt
timestart=$(date +%s.%N)
./testeth -- --all --exectimelog
timeend=$(date +%s.%N)

#Send Mails
RECIPIENTS="dimitry@ethereum.org pawel@ethereum.org chris@ethereum.org andrei@ethereum.org"
date=$(date +%Y-%m-%d)
(
echo "REPORT"
exectime=$(echo "$timeend - $timestart" | bc)
echo "Test execution time: $exectime s"
cat $workdir/testlog.txt
cat $workdir/buildlog.txt
) | mail -s "cpp-ethereum test results $date" $RECIPIENTS
