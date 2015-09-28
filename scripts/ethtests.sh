#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# This script should get a project name from jenkins and figure out which tests to run

if [[ $# -ne 1 ]]; then
	echo "RUNTESTS - ERROR: Expected 1 argument for the run tests script but got $#!"
	exit 1
fi

TEST_EXEC="NONE"
REPOS_TEST_MAP=("webthree-helpers:NONE"
		"tests:NONE"
		"libweb3core:NONE"
		"libethereum:libethereum/build/test/testeth"
		"libwhisper:NONE"
		"webthree:webthree/build/test/testweb3"
		"web3.js:NONE"
		"solidity:solidity/build/test/soltest"
		"alethzero:NONE"
		"mix:NONE")

function get_repo_testexec() {
	if [[ $1 == "" ]]; then
		echo "ETHTESTS - ERROR: get_repo_testexec() function called without an argument."
		exit 1
	fi
	for repo in "${REPOS_TEST_MAP[@]}" ; do
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

case $1 in
	"libweb3core")
		TEST_REPOSITORIES=(libethereum webthree solidity)
		;;
	"alethzero")
		echo "ETHTESTS - INFO: \"$1\" contains no tests. No worries."
		exit 0
		;;
	"mix")
		echo "ETHTESTS - INFO: \"$1\" contains no tests. No worries."
		exit 0
		;;
	"libethereum")
		TEST_REPOSITORIES=(libethereum webthree solidity)
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

for repository in "${TEST_REPOSITORIES[@]}"
do
	get_repo_testexec $repository
	if [[ $TEST_EXEC == "NONE" ]]; then
		echo "ETHTESTS - INFO: Repository $repository has no tests, so we are skipping it for project \"$1\"."
		continue;
	fi
	echo "ETHTESTS - INFO: Will run test file ${TEST_EXEC} for project \"$1\"."
	# remove old test results
	rm -rf ${repository}_results.xml
	# run tests
	./$TEST_EXEC --log_format=XML --log_sink=${repository}_results.xml --log_level=all --report_level=no
done
