#!/usr/bin/env bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to update the different ethereum repositories to latest develop
# Invoke from the root directory and make sure you have the arguments set as explained
# in the usage string.


# Get SCRIPT_DIR, the directory the script is located even if there are symlinks involved
FILE_SOURCE="${BASH_SOURCE[0]}"
# resolve $FILE_SOURCE until the file is no longer a symlink
while [ -h "$FILE_SOURCE" ]; do
	SCRIPT_DIR="$( cd -P "$( dirname "$FILE_SOURCE" )" && pwd )"
	FILE_SOURCE="$(readlink "$FILE_SOURCE")"
	# if $FILE_SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
	[[ $FILE_SOURCE != /* ]] && FILE_SOURCE="$SCRIPT_DIR/$FILE_SOURCE"
done
SCRIPT_DIR="$( cd -P "$( dirname "$FILE_SOURCE" )" && pwd )"

# Now that we got the directory, source some common functionality
source "${SCRIPT_DIR}/ethbuildcommon.sh"

USE_ALL=0
ROOT_DIR=$(pwd)
NO_PUSH=0
USE_SSH=0
DO_SIMPLE_PULL=0
SHALLOW_FETCH=""
UPSTREAM=upstream
ORIGIN=origin
REQUESTED_BRANCH=develop
WEB3JS_BRANCH=master
REQUESTED_ARG=""
REQUESTED_PROJECT=""
REPO_URL=""
BUILD_PR="none"
REPOS_MAP=("webthree-helpers:https://github.com/ethereum/webthree-helpers"
	   "tests:https://github.com/ethereum/tests"
	   "libweb3core:https://github.com/ethereum/libweb3core"
	   "libethereum:https://github.com/ethereum/libethereum"
	   "libwhisper:https://github.com/ethereum/libwhisper"
	   "webthree:https://github.com/ethereum/webthree"
	   "web3.js:https://github.com/ethereum/web3.js"
	   "solidity:https://github.com/ethereum/solidity"
	   "alethzero:https://github.com/ethereum/alethzero"
	   "mix:https://github.com/ethereum/mix")
REPOS_SSH_MAP=("webthree-helpers:git@github.com:ethereum/webthree-helpers.git"
	   "tests:git@github.com:ethereum/tests.git"
	   "libweb3core:git@github.com:ethereum/libweb3core.git"
	   "libethereum:git@github.com:ethereum/libethereum.git"
	   "libwhisper:git@github.com:ethereum/libwhisper.git"
	   "webthree:git@github.com:ethereum/webthree.git"
	   "web3.js:git@github.com:ethereum/web3.js.git"
	   "solidity:git@github.com:ethereum/solidity.git"
	   "alethzero:git@github.com:ethereum/alethzero.git"
	   "mix:git@github.com:ethereum/mix.git")

function get_repo_url() {
	if [[ $1 == "" ]]; then
		echo "ETHUPDATE - ERROR: get_repo_url() function called without an argument."
		exit 1
	fi
	REPOS_MAP_VAR="${REPOS_MAP[@]}"
	if [[ $USE_SSH -eq 1 ]]; then
		REPOS_MAP_VAR="${REPOS_SSH_MAP[@]}"
	fi
	for repo in $REPOS_MAP_VAR ; do
		KEY=${repo%%:*}
		if [[ $KEY == $1 ]]; then
			REPO_URL=${repo#*:}
			break
		fi
	done
	if [[ $REPO_URL == "" ]]; then
		echo "ETHUPDATE - ERROR: Requested url of unknown repo: ${1}."
		exit 1
	fi
}

# Takes a branch value and if it is not empty or "null" sets the requested branch to it
# "null" can come from the way jenkins handles empty variables
function set_requested_branch() {
	if [[ $1 != "" && $1 != "null" ]]; then
		REQUESTED_BRANCH=$1
	fi
}

# Takes a repository as an argument and using the environment variables tries to see if
# the requested branch exists for that repo and if it does it sets it
function get_repo_branch() {
	if [[ $1 == "" ]]; then
		echo "ETHUPDATE - ERROR: get_repo_branch() function called without an argument."
		exit 1
	fi
	# get the name of the Requested Branch
	REQUESTED_BRANCH="develop"
	case $1 in
		"webthree-helpers")
			set_requested_branch $WEBTHREEHELPERS_BRANCH
			;;
		"libweb3core")
			set_requested_branch $LIBWEB3CORE_BRANCH
			;;
		"libethereum")
			set_requested_branch $LIBETHEREUM_BRANCH
			;;
		"webthree")
			set_requested_branch $WEBTHREE_BRANCH
			;;
		"web3.js")
			set_requested_branch $WEB3JS_BRANCH
			;;
		"solidity")
			set_requested_branch $SOLIDITY_BRANCH
			;;
		"alethzero")
			set_requested_branch $ALETHZERO_BRANCH
			;;
		"mix")
			set_requested_branch $MIX_BRANCH
			;;
		"tests")
			set_requested_branch $TESTS_BRANCH
			;;
		*)
			echo "ETHUPDATE - ERROR: Unrecognized repo argument at get_repo_branch() \"$1\".";
			exit 1
			;;
	esac
	echo "ETHUPDATE - INFO: get_repo_branch() setting branch for $1 to ${REQUESTED_BRANCH}."
	#if not develop or master we got some work to do
	if [[ $REQUESTED_BRANCH != "develop" && $REQUESTED_BRANCH != "master" ]]; then
		if [[ $GH_PR_USER == "" ]];then
			GH_PR_USER="ethereum"
			echo "ETHUPDATE - INFO: get_repo_branch() no GH_PR_USER given, defaulting to ethereum repo."
		fi
		#fetch the requested branch
		git fetch https://github.com/${GH_PR_USER}/$1 ${REQUESTED_BRANCH}
		if [[ $? -ne 0 ]]; then
			if [[ $GH_PR_USER != "ethereum" ]]; then
				echo "ETHUPDATE - WARNING: Could not find ${REQUESTED_BRANCH} of ${1} from the fork of ${GH_PR_USER}. Trying the ethereum upstream"
				git fetch https://github.com/ethereum/$1 ${REQUESTED_BRANCH}
				if [[ $? -eq 0 ]]; then
					git checkout FETCH_HEAD
					echo "ETHUPDATE - INFO: Found ${REQUESTED_BRANCH} of ${1} in the ethereum upstream"
					return
				fi
			fi
			echo "ETHUPDATE - ERROR: Could not fetch ${REQUESTED_BRANCH} of ${1} for ${GH_PR_USER} or from the ethereum upstream.. Defaulting to develop."
			REQUESTED_BRANCH="develop"
                else
			git checkout FETCH_HEAD
		fi
	fi
}

function print_help {
	echo "Usage: ethupdate.sh [options]"
	echo "Arguments:"
	echo "    --help                  Will print this help message."
	echo "${PROJECTS_HELP}"
	echo "    --branch NAME           Will update to the specified branch. Default is ${REQUESTED_BRANCH}."
	echo "    --origin NAME           Will send the updates back to origin NAME if specified."
	echo "    --upstream NAME         The name of the remote to pull from. Default is ${UPSTREAM}."
	echo "    --no-push               Don't push anything back to origin."
	echo "    --use-ssh               Use ssh to clone the repos instead of https."
	echo "    --shallow-fetch         Perform git clone and git fetch with --depth=1."
	echo "    --simple-pull           If a branch is given but can't be checked out, then give this argument to attemt a simple git pull"
	echo "    --build-pr HEX          Will make sure that the main repository for the project has the commit of a particular PR checked out. You can also give the value of none to disable this argument."
	echo "    --all                   In addition to cloning the repositores needed to build this project, also clone all projects that depend on it"
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
			"project")
				set_repositories "ETHUPDATE" $arg
				;;
			"build-pr")
				BUILD_PR=$arg
				;;
			*)
				echo "ETHUPDATE - ERROR: Unrecognized argument \"$arg\".";
				print_help
				exit 1
		esac
		REQUESTED_ARG=""
		continue
	fi

	if [[ $arg == "--help" ]]; then
		print_help
		exit 1
	fi

	if [[ $arg == "--branch" ]]; then
		REQUESTED_ARG="branch"
		continue
	fi

	if [[ $arg == "--project" ]]; then
		REQUESTED_ARG="project"
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

	if [[ $arg == "--build-pr" ]]; then
		REQUESTED_ARG="build-pr"
		continue
	fi

	if [[ $arg == "--no-push" ]]; then
		NO_PUSH=1
		continue
	fi

	if [[ $arg == "--use-ssh" ]]; then
		USE_SSH=1
		continue
	fi

	if [[ $arg == "--shallow-fetch" ]]; then
		SHALLOW_FETCH=" --depth=1"
		continue
	fi

	if [[ $arg == "--simple-pull" ]]; then
		DO_SIMPLE_PULL=1
		continue
	fi

	if [[ $arg == "--all" ]]; then
		USE_ALL=1
		continue
	fi

	echo "ETHUPDATE - ERROR: Unrecognized argument \"$arg\".";
	print_help
	exit 1
done

if [[ ${REQUESTED_ARG} != "" ]]; then
	echo "ETHUPDATE - ERROR: Expected value for the \"${REQUESTED_ARG}\" argument";
	exit 1
fi

if [[ $USE_ALL -eq 1 && ($REQUESTED_PROJECT != "alethzero" || $REQUESTED_PROJECT != "mix") ]]; then
	CLONE_REPOSITORIES=("${ALL_CLONE_REPOSITORIES[@]}")
fi

for repository in "${CLONE_REPOSITORIES[@]}"
do
	CHECKOUT_HEX=0
	# note if we need to checkout a PR's commit
	if [[ $repository == $REQUESTED_PROJECT && $BUILD_PR != "none" ]]; then
		CHECKOUT_HEX=1
	fi

	echo "ETHUPDATE - INFO: Starting update process of ${repository} for requested project ${REQUESTED_PROJECT}";
	CLONED_THE_REPO=0
	cd $repository >/dev/null 2>/dev/null
	if [[ $? -ne 0 ]]; then
		if [[ $REQUESTED_PROJECT == "" ]]; then
			echo "ETHUPDATE - INFO: Skipping ${repository} because directory does not exit";
			cd $ROOT_DIR
			continue
		else
			echo "ETHUPDATE - INFO: Repository ${repository} for requested project ${REQUESTED_PROJECT} did not exist. Cloning ..."
			get_repo_url $repository
			git clone --recursive $REPO_URL $SHALLOW_FETCH
			CLONED_THE_REPO=1
			cd $repository >/dev/null 2>/dev/null
		fi
	fi
	BRANCH="$(git symbolic-ref HEAD 2>/dev/null)" ||
		BRANCH="(unnamed branch)"     # detached HEAD
	BRANCH=${BRANCH##refs/heads/}

	get_repo_branch $repository
	# if the "none" value was given then checkout and pull requested branch
	if [[ $BUILD_PR == "none" ]]; then
		git checkout -f $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ETHUPDATE - ERROR: Could not checkout \"${REQUESTED_BRANCH}\" for ${repository}."
			exit 1
		else
			echo "ETHUPDATE - INFO: Checked out branch ${REQUESTED_BRANCH} for repository ${repository}."
		fi
	else
		get_repo_branch $repository
	fi

	# if we need to checkout specific commit for a PR do so
	if [[ $CHECKOUT_HEX -eq 1 ]]; then
		echo "ETHUPDATE - INFO: Checking out commit ${BUILD_PR} for ${repository} as requested."
		get_repo_url $repository
		git fetch --tags --progress $REPO_URL +refs/pull/*:refs/remotes/origin/pr/*
		git checkout -f $BUILD_PR
		# Init new and update submodules
		git submodule update --init --force
		cd $ROOT_DIR
		continue
	elif [[ $DO_SIMPLE_PULL -eq 1 ]]; then
		echo "ETHUPDATE - INFO: Performing simple pull for ${repository} ..."
		git pull $SHALLOW_FETCH
		if [[ $? -ne 0 ]]; then
			echo "ETHUPDATE - ERROR: Doing a simple pull for ${repository} failed. Skipping this repository ..."
		fi
		# Init new and update submodules
		git submodule update --init --force
		cd $ROOT_DIR
		continue
	fi

	if [[ $REQUESTED_BRANCH != "develop" && $REQUESTED_BRANCH != "master" ]];then
		#by this point we should have succesfully fetched the branch from the remote so just check it out
		git checkout -f $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ETHUPDATE - ERROR: Could not check out branch ${REQUESTED_BRANCH} for repository ${repository}. Skipping ..."
		else
			echo "ETHUPDATE - INFO: Checked out branch ${REQUESTED_BRANCH} for repository ${repository}."
		fi
		# Init new and update submodules
		git submodule update --init --force
		cd $ROOT_DIR
		continue
	fi

	# Pull changes from what the user set as the upstream repository, unless it's just been cloned
	if [[ $CLONED_THE_REPO -eq 0 ]]; then
		if [[ $REQUESTED_BRANCH == "develop" || $REQUESTED_BRANCH == "master" ]]; then
			# We get here if no special branch was requested, so make sure we got the non-special
			# branch checked out before pulling
			echo "ETHUPDATE - INFO: Make sure we are in $REQUESTED_BRANCH"
			git checkout -f $REQUESTED_BRANCH
		fi
		git pull $UPSTREAM $REQUESTED_BRANCH $SHALLOW_FETCH
		# Init new and update submodules
		git submodule update --init --force
	else
		# if just cloned, make a local branch tracking the origin's requested branch
		git fetch origin $SHALLOW_FETCH
		if [[ $BRANCH != $REQUESTED_BRANCH ]]; then
			git checkout -f --track -b $REQUESTED_BRANCH origin/$REQUESTED_BRANCH
		fi
	fi

	if [[ $? -ne 0 ]]; then
		if [[ $DO_SIMPLE_PULL -eq 1 ]]; then
			echo "ETHUPDATE - INFO: ${repository} failed to pull ${REQUESTED_BRANCH}. Performing a simple pull anyway ..."
			git pull $SHALLOW_FETCH
			if [[ $? -ne 0 ]]; then
				echo "ETHUPDATE - ERROR: Doing a simple pull for ${repository} failed. Skipping this repository ..."
			fi
			# Init new and update submodules
			git submodule update --init --force
		else
			echo "ETHUPDATE - ERROR: Pulling changes for repository ${repository} from ${UPSTREAM} into the ${REQUESTED_BRANCH} branch failed."
		fi
		cd $ROOT_DIR
		continue
	fi
	# If upstream and origin are not the same, push the changes back to origin and no push has not been asked
	if [[ $NO_PUSH -eq 0 && $UPSTREAM != $ORIGIN ]]; then
		git push $ORIGIN $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ETHUPDATE - ERROR: Could not update origin ${ORIGIN} of repository ${repository} for the ${REQUESTED_BRANCH}."
			cd $ROOT_DIR
			continue
		fi
	fi
	echo "ETHUPDATE - INFO: ${repository} succesfully updated!"
	cd $ROOT_DIR
done
