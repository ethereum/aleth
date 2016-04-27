#!/usr/bin/env bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# This script should get a project name from jenkins and figure out which tests to run

set -e

if [[ $# -ne 1 && $# -ne 2 ]]; then
	echo "RUNTESTS - ERROR: Expected either 1 or 2 arguments for the run tests script but got $#!"
	exit 1
fi

RUN_FROM_UMBRELLA=0
TEST_EXEC="NONE"
REPOS_TEST_MAP=("webthree-helpers:NONE"
		"tests:NONE"
		"libweb3core:libweb3core/build/test/testweb3core"
		"libethereum:libethereum/build/test/testeth"
		"libethereum_vmjit:libethereum/build/test/testeth --vm jit"
		"libethereum_vmsmart:libethereum/build/test/testeth --vm smart"
		"libwhisper:NONE"
		"webthree:webthree/build/test/testweb3"
		"web3.js:NONE"
		"solidity:solidity/build/test/soltest"
		"alethzero:NONE"
		"mix:NONE")

UMBRELLA_REPOS_TEST_MAP=("webthree-helpers:NONE"
		"tests:NONE"
		"libweb3core:build/libweb3core/test/testweb3core"
		"libethereum:build/libethereum/test/testeth"
		"libethereum_vmjit:build/libethereum/test/testeth --vm jit"
		"libethereum_vmsmart:build/libethereum/test/testeth --vm smart"
		"libwhisper:NONE"
		"webthree:build/webthree/test/testweb3"
		"web3.js:NONE"
		"solidity:build/solidity/test/soltest"
		"alethzero:NONE"
		"mix:NONE")

function get_repo_testexec() {
	if [[ $1 == "" ]]; then
		echo "ETHTESTS - ERROR: get_repo_testexec() function called without an argument."
		exit 1
	fi
	REPOS_TEST_VAR="${REPOS_TEST_MAP[@]}"
	if [[ $RUN_FROM_UMBRELLA -eq 1 ]]; then
		REPOS_TEST_VAR="${UMBRELLA_REPOS_TEST_MAP[@]}"
	fi
	for repo in $REPOS_TEST_VAR ; do
		KEY=${repo%%:*}
		if [[ $KEY == $1 ]]; then
			TEST_EXEC=${repo#*:}
			break
		fi
	done
	if [[ $TEST_EXEC == "" ]]; then
		echo "ETHTESTS - ERROR: Requested test executable of unknown repo: ${1}."
		exit 1
	fi
}

if [[ $2 != "" ]]; then
	if [[ $2 != "--umbrella" ]]; then
		echo "ETHTESTS - ERROR: Second argument can only be --umbrella if tests are run from the umbrella repo"
		exit 1
	fi
	RUN_FROM_UMBRELLA=1
fi

case $1 in
	"all")
		TEST_REPOSITORIES=(libweb3core libethereum libethereum_vmjit libethereum_vmsmart webthree solidity)
		;;
	"webthree-helpers")
		echo "ETHTESTS - INFO: Not running any tests for project \"$1\"."
		exit 0
		;;
	"libweb3core")
		TEST_REPOSITORIES=(libweb3core libethereum libethereum_vmjit libethereum_vmsmart webthree solidity)
		;;
	"alethzero")
		echo "ETHTESTS - INFO: Not running any tests for project \"$1\"."
		exit 0
		;;
	"mix")
		echo "ETHTESTS - INFO: Not running any tests for project \"$1\"."
		exit 0
		;;
	"libethereum")
		TEST_REPOSITORIES=(libethereum libethereum_vmsmart libethereum_vmjit webthree solidity)
		;;
	"webthree")
		TEST_REPOSITORIES=(webthree solidity)
		;;
	"solidity")
		TEST_REPOSITORIES=(solidity)
		;;
	*)
		echo "ETHTESTS - ERROR: Unrecognized value \"$1\" for the project name"
		exit 1
		;;
esac

# Set a special environment variable for use by evmjit tests
# export EVMJIT="-cache=0"

# remove all old test results
rm -rf *_results.xml
echo "ETHTESTS - INFO: We are about to run the tests from ${PWD}"
for repository in "${TEST_REPOSITORIES[@]}"
do
	get_repo_testexec $repository
	if [[ $TEST_EXEC == "NONE" ]]; then
		echo "ETHTESTS - INFO: Repository $repository has no tests, so we are skipping it for project \"$1\"."
		continue;
	fi
	echo "ETHTESTS - INFO: Will run test file ${TEST_EXEC} for project \"$1\"."
	# run tests
	./$TEST_EXEC --log_format=XML --log_sink=${repository}_results.xml --log_level=all --report_level=no
done
