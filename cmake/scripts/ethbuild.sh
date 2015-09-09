#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to build the different ethereum repositories. For usage look
# at the string echoed in the very beginning.

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


REPO_MSVC_SLN=""
REPOS_MSVC_SLN_MAP=("cpp-ethereum:ethereum.sln"
	"solidity:solidity.sln")

function get_repo_sln() {
	if [[ $1 == "" ]]; then
		echo "ETHUPDATE - ERROR: get_repo_sln() function called without an argument."
		exit 1
	fi
	for repo in "${REPOS_MSVC_SLN_MAP[@]}" ; do
		KEY=${repo%%:*}
		if [[ $KEY =~ $1 ]]; then
			REPO_MSVC_SLN=${repo#*:}
			break
		fi
	done
	if [[ $REPO_MSVC_SLN == "" ]]; then
		echo "ETHBUILD - ERROR: Requested MSVC Solution of unknown repo: ${1}."
		exit 1
	fi
}

ROOT_DIR=$(pwd)
REQUESTED_BRANCH=develop
CLEAN_BUILD=0
BUILD_TYPE=RelWithDebInfo
REQUESTED_ARG=""
EXTRA_BUILD_ARGS=""
MAKE_CORES=1
NOGIT=0

function print_help {
	echo "Usage: ethbuild.sh [extra-options]"
	echo "Arguments:"
	echo "    --help                  Print this help message."
	echo "    --clean-build           If given the build directories will be cleared."
	echo "${PROJECTS_HELP}"
	echo "    --no-git                If given no git branch check and no git checkout will be performed."
	echo "    --branch NAME           The branch requested to build. Default is ${REQUESTED_BRANCH}."
	echo "    --build-type BUILDTYPE  If given then this is gonna be the value of -DCMAKE_BUILD_TYPE. Default is ${BUILD_TYPE} "
	echo "    --cores NUMBER          The value to the cores argument of make. e.g.: make -j4. Default is ${MAKE_CORES}."
}

for arg in ${@:1}
do
	if [[ ${REQUESTED_ARG} != "" ]]; then
	case $REQUESTED_ARG in
		"build-type")
			BUILD_TYPE=$arg
			;;
		"branch")
			REQUESTED_BRANCH=$arg
			;;
		"make-cores")
			re='^[0-9]+$'
			if ! [[ $arg =~ $re ]] ; then
				echo "ERROR: Value for --cores is not a number!"
				exit 1
			fi
			MAKE_CORES=$arg
			;;
		"project")
			set_repositories "ETHBUILD" $arg
			;;
		*)
			echo "ERROR: Unrecognized argument \"$arg\".";
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

	if [[ $arg == "--clean-build" ]]; then
		CLEAN_BUILD=1
		continue
	fi

	if [[ $arg == "--no-git" ]]; then
		NOGIT=1
		continue
	fi

	if [[ $arg == "--project" ]]; then
		REQUESTED_ARG="project"
		continue
	fi

	if [[ $arg == "--build-type" ]]; then
		REQUESTED_ARG="build-type"
		continue
	fi

	if [[ $arg == "--branch" ]]; then
		REQUESTED_ARG="branch"
		continue
	fi

	if [[ $arg == "--cores" ]]; then
		REQUESTED_ARG="make-cores"
		continue
	fi

	# all other arguments will just be given as they are to cmake
	EXTRA_BUILD_ARGS+="$arg "
	REQUESTED_ARG=""
done

if [[ ${REQUESTED_ARG} != "" ]]; then
	echo "ERROR: Expected value for the \"${REQUESTED_ARG}\" argument";
	exit 1
fi

for repository in "${BUILD_REPOSITORIES[@]}"
do
	cd $repository >/dev/null 2>/dev/null
	if [[ $? -ne 0 ]]; then
		echo "Skipping ${repository} because directory does not exit";
		# Go back to root directory
		cd $ROOT_DIR
		continue
	fi

	if [[ $NOGIT -eq 0 ]]; then
		BRANCH="$(git symbolic-ref HEAD 2>/dev/null)" ||
		BRANCH="(unnamed branch)"	  # detached HEAD
		BRANCH=${BRANCH##refs/heads/}
	fi

	if [[ $NOGIT -eq 0 && $BRANCH != $REQUESTED_BRANCH ]]; then
		echo "BUILD WARNING: ${repository} was in ${BRANCH} branch, while building on ${REQUESTED_BRANCH} was requested.";
		git checkout $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ERROR: Checking out branch ${REQUESTED_BRANCH} for ${repository} failed";
			exit 1
		fi
	fi

	# Make/clean build directory depending on requested arguments
	if [[ -d "build" ]]; then
		if [[ $CLEAN_BUILD -eq 1 ]]; then
			rm -rf build
			mkdir build
		fi
	else
		mkdir build
	fi

	# Go in build directory for the repository and configure/build
	cd build
	if [[ $OSTYPE == "cygwin" ]]; then
		# For Windows we gotta emulate the Visual studio environment
		source "${SCRIPT_DIR}/ethwindowsenv.sh"
		cmake .. -G "Visual Studio 12 2013 Win64"
		if [[ $? -ne 0 ]]; then
			echo "ERROR: cmake configure phase for repository \"$repository\" failed.";
			exit 1
		fi

		get_repo_sln $repository
		"${MSBUILD_EXECUTABLE}" $REPO_MSVC_SLN /p:Configuration=$BUILD_TYPE /m:${MAKE_CORES}
		if [[ $? -ne 0 ]]; then
			echo "ERROR: Building repository \"$repository\" failed.";
			exit 1
		fi
	else
		cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE $EXTRA_BUILD_ARGS;
		if [[ $? -ne 0 ]]; then
			echo "ERROR: cmake configure phase for repository \"$repository\" failed.";
			exit 1
		fi

		make -j${MAKE_CORES}
		if [[ $? -ne 0 ]]; then
			echo "ERROR: Building repository \"$repository\" failed.";
			exit 1
		fi
	fi
	# Go back to root directory
	cd $ROOT_DIR
done
