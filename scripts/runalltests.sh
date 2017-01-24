workdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"/testslog
testdir=$workdir/../../../tests

if [ -d $workdir ]; then
rm -r $workdir
fi
mkdir $workdir
exec &> $workdir/testlog.txt
cd $testdir

echo "Fetching the test repository:"
git pull
echo "Test Repository last commit hash:"
git rev-parse HEAD
cd ..
cd cpp-ethereum
echo "Fetching the cpp-ethereum develop:"
git pull
echo "cpp-ethereum last commit hash:"
git rev-parse HEAD
cd build
echo "Make cpp-ethereum develop:"
make -j8

exec &>> $workdir/testlog.txt
cd test
./testeth --all --exectimelog

exec &> /dev/null
#cd $workdir
#rm results.zip
#zip results.zip *.txt
date=$(date +%Y-%m-%d)

mail -s "cpp-ethereum test results "$date dimitry@ethereum.org < $workdir/testlog.txt  #-A $workdir/results.zip
mail -s "cpp-ethereum test results "$date chris@ethereum.org < $workdir/testlog.txt #-A $workdir/results.zip
rm -r $workdir
