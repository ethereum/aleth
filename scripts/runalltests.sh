#!/usr/bin/env bash

workdir=$(pwd)

#Clean the previous build
rm -rf aleth 2>/dev/null || true
rm -rf tests 2>/dev/null || true
exec &> $workdir/buildlog.txt
export ETHEREUM_TEST_PATH="$(pwd)/tests"

#Clonning Repositories
echo "Cloning Repositories"
git clone --depth 1 --single-branch https://github.com/ethereum/tests.git
git clone --recursive --depth 1 --single-branch https://github.com/ethereum/aleth.git
cd tests
testHead=$(git rev-parse HEAD)
cd ..
cd tests/RPCTests
npm install
cd $workdir/aleth
cppHead=$(git rev-parse HEAD)

#Prepare test results
mkdir build
cd build
echo "Make aleth develop:"
cmake .. -DCOVERAGE=On
make -j8
echo "Running all tests:"
echo "aleth repository at commit $cppHead"
echo "tests repository at commit $testHead"
exec 2> $workdir/testlog.txt
timestart=$(date +%s.%N)
TMPDIR=/dev/shm
test/testeth -- --all --exectimelog
cd $workdir/tests/RPCTests
echo "#--------------RPC TESTS--------------"
node main.js $workdir/aleth/build/eth/eth
timeend=$(date +%s.%N)
date=$(date +%Y-%m-%d)

# Upload coverage report
if [ -z "$CODECOV_TOKEN" ]; then
    echo "Warning! CODECOV_TOKEN not set. See https://codecov.io/gh/ethereum/aleth/settings."
else
    bash <(curl -s https://codecov.io/bash) -n alltests -b "$date" -F alltests -a '>/dev/null 2>&1'
fi

# Make report
cd $workdir
(
echo "REPORT"
exectime=$(echo "$timeend - $timestart" | bc)
echo "Test execution time: $exectime s"
echo "Coverage: https://codecov.io/gh/ethereum/aleth/commit/$cppHead"
cat testlog.txt
cat buildlog.txt
) > report.txt

# Send mail
RECIPIENTS="dimitry@ethereum.org pawel@ethereum.org chris@ethereum.org andrei@ethereum.org yoichi@ethereum.org"
mail < report.txt -s "aleth alltests $date" $RECIPIENTS
