#!/usr/bin/env bash
# Originally written by Dimitriy <dimitry@ethdev.com>
# Modifications by Lefteris Karapetss <lefteris@refu.co>
#
# Script to run coverage tests on the umbrella project.

BUILD_DIR=webthree-umbrella/build
TEST_MODE=""
TEST_TARGETS=()
TEST_LIBS=()

for i in "$@"
do
case $i in
	-builddir)
	shift
	((i++))
	BUILD_DIR=${!i}
	shift
	;;
	--all)
	TEST_MODE="--all"
	shift
	;;
	--filltests)
	TEST_FILL="--filltests"
	shift
	;;
	--libethereum)
	TEST_TARGETS+=("libethereum/test/testeth")
	TEST_LIBS+=(libethereum)
	shift
	;;
	# --libweb3core)
	# TEST_TARGETS+=("libweb3core/test/testweb3core")
	# TEST_LIBS+=(libweb3core)
	# shift
	# ;;
	--solidity)
	TEST_TARGETS+=("solidity/test/soltest")
	TEST_LIBS+=(solidity)
	shift
	;;
	--webthree)
	TEST_TARGETS+=("webthree/test/testweb3")
	TEST_LIBS+=(webthree)
	shift
	;;
esac
done

if [ ${#TEST_LIBS[*]} -eq 0 ]
then
	# TEST_LIBS=(libethereum libweb3core solidity webthree)
	# TEST_TARGETS=("libethereum/test/testeth" "libweb3core/test/testweb3core" "solidity/test/soltest" "webthree/test/testweb3")
	TEST_LIBS=(libethereum solidity webthree)
	TEST_TARGETS=("libethereum/test/testeth" "solidity/test/soltest" "webthree/test/testweb3")
fi

#remove repeats
TEST_LIBS=($(printf "%s\n" "${TEST_LIBS[@]}" | sort -u))

if  !(hash lcov 2>/dev/null); then
    echo "ETHCOVERAGE -- ERROR: lcov not found";
    exit 1;
fi
if  !(hash genhtml 2>/dev/null); then
    echo "ETHCOVERAGE -- ERROR: genthml not found";
fi

for index in ${!TEST_LIBS[*]}
do
	echo "ETHCOVERAGE -- INFO: Starting coverage work for \"${index}\"..."
	TEST_LIB=${TEST_LIBS[$index]}
	TEST_TARGET=${TEST_TARGETS[$index]}

	which $BUILD_DIR/$TEST_TARGET >/dev/null 2>&1
	if [[ $? != 0 ]]; then
	    echo "ETHCOVERAGE -- ERROR: Need to compile with -DPROFILING=1"
	    exit 1
	fi

	mkdir $BUILD_DIR/testcoverage
	OUTPUT_DIR=$BUILD_DIR/testcoverage/$TEST_LIB


	echo "ETHCOVERAGE -- INFO: Cleaning previous report..."
	if [[ -d "$OUTPUT_DIR" ]]; then
	    rm -r $OUTPUT_DIR
	fi
	mkdir $OUTPUT_DIR
	lcov --directory $BUILD_DIR --zerocounters
	lcov --capture --initial --directory $BUILD_DIR --output-file $OUTPUT_DIR/coverage_base.info

	echo "ETHCOVERAGE -- INFO: Running tests..."
	$BUILD_DIR/$TEST_TARGET $TEST_MODE $TEST_FILL

	echo "ETHCOVERAGE -- INFO: Preparing coverage info..."
	lcov --capture --directory $BUILD_DIR --output-file $OUTPUT_DIR/coverage_test.info
	lcov --add-tracefile $OUTPUT_DIR/coverage_base.info --add-tracefile $OUTPUT_DIR/coverage_test.info --output-file $OUTPUT_DIR/coverage_all.info
	lcov --extract $OUTPUT_DIR/coverage_all.info '*'$TEST_LIB'/*' --output-file $OUTPUT_DIR/coverage_export.info
	genhtml $OUTPUT_DIR/coverage_export.info --output-directory $OUTPUT_DIR/export
done
