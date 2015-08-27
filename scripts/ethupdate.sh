#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to update the different ethereum repositories to latest develop
# Invoke from the root directory and make sure you have the arguments set as explained
# in the usage string.

ROOT_DIR=$(pwd)
PROJECTS=(cpp-ethereum cpp-ethereum-cmake webthree solidity alethzero mix)
UPSTREAM=upstream
ORIGIN=origin
REQUESTED_BRANCH=develop
REQUESTED_ARG=""

function print_help {
	echo "Usage: ethupdate.sh [options]"
	echo "    --branch NAME Will update to the specified branch."
	echo "    --origin NAME Will send the updates back to origin NAME if specified."
	echo "    --upstream NAME The name of the remote to pull from."
}

for arg in ${@:1}
do
	if [[ ${REQUESTED_ARG} != "" ]]; then
		case $REQUESTED_ARG in
			"origin")
				ORIGIN=$arg
				;;
			"upstream")
				UPSTREAM=$arg
				;;
			"branch")
				REQUESTED_BRANCH=$arg
				;;
			*)
				echo "ERROR: Unrecognized argument \"$arg\".";
				print_help
				exit 1
		esac
		REQUESTED_ARG=""
		continue
	fi

	if [[ $arg == "--branch" ]]; then
		REQUESTED_ARG="branch"
		continue
	fi

	if [[ $arg == "--origin" ]]; then
		REQUESTED_ARG="origin"
		continue
	fi

	if [[ $arg == "--upstream" ]]; then
		REQUESTED_ARG="upstream"
		continue
	fi

	echo "ERROR: Unrecognized argument \"$arg\".";
	print_help
	exit 1
done

if [[ ${REQUESTED_ARG} != "" ]]; then
	echo "ERROR: Expected value for the \"${REQUESTED_ARG}\" argument";
	exit 1
fi

for project in "${PROJECTS[@]}"
do
	cd $project >/dev/null 2>/dev/null
	if [[ $? -ne 0 ]]; then
		echo "Skipping ${project} because directory does not exit";
		cd $ROOT_DIR
		continue
	fi
	BRANCH="$(git symbolic-ref HEAD 2>/dev/null)" ||
		BRANCH="(unnamed branch)"     # detached HEAD
	BRANCH=${BRANCH##refs/heads/}
	if [[ $BRANCH != $REQUESTED_BRANCH ]]; then
		echo "WARNING: Not updating ${project} because it's not in the ${REQUESTED_BRANCH} branch"
		cd $ROOT_DIR
		continue
	fi

	# Pull changes from what the user set as the upstream repository
	git pull $UPSTREAM $REQUESTED_BRANCH
	if [[ $? -ne 0 ]]; then
		echo "ERROR: Pulling changes for project ${project} from ${UPSTREAM} into the ${REQUESTED_BRANCH} branch failed."
		cd $ROOT_DIR
		continue
	fi
	# If upstream and origin are not the same, push the changes back to origin
	if [[ $UPSTREAM != $ORIGIN ]]; then
		git push $ORIGIN $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ERROR: Could not update origin ${ORIGIN} of project ${project} for the ${REQUESTED_BRANCH}."
			cd $ROOT_DIR
			continue
		fi
	fi
	echo "${project} succesfully updated!"
	cd $ROOT_DIR
done
