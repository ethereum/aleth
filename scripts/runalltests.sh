#!/usr/bin/env bash

workdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#Clean the previous build
cd $workdir
rm -r cpp-ethereum
rm -r tests
exec &> $workdir/buildlog.txt
ETHEREUM_TEST_PATH=$workdir/tests

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
cmake .. -DCOVERAGE=On
make -j8
echo "Running all tests:"
echo "cpp-ethereum repository at commit $cppHead"
echo "tests repository at commit $testHead"
exec 2> $workdir/testlog.txt
timestart=$(date +%s.%N)
test/testeth -- --all --exectimelog
timeend=$(date +%s.%N)
date=$(date +%Y-%m-%d)

# Upload coverage report
if [ -z "$CODECOV_TOKEN" ]; then
    echo "Warning! CODECOV_TOKEN not set. See https://codecov.io/gh/ethereum/cpp-ethereum/settings."
else
    bash <(curl -s https://codecov.io/bash) -n alltests -b "$date" -F alltests -a '>/dev/null 2>&1'
fi


#Send Mails
RECIPIENTS="dimitry@ethereum.org pawel@ethereum.org chris@ethereum.org andrei@ethereum.org"
(
echo "REPORT"
exectime=$(echo "$timeend - $timestart" | bc)
echo "Test execution time: $exectime s"
echo "Coverage analyze: https://codecov.io/gh/ethereum/cpp-ethereum/commit/$cppHead"
cat $workdir/testlog.txt
cat $workdir/buildlog.txt
) | mail -s "cpp-ethereum test results $date" $RECIPIENTS
