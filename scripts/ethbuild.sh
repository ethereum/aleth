#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to build the different ethereum repositories. For usage look
# at the string echoed in the very beginning.

PROJECTS=(cpp-ethereum webthree solidity alethzero mix)
ROOT_DIR=$(pwd)
REQUESTED_BRANCH=develop
CLEAN_BUILD=0
BUILD_TYPE=RelWithDebInfo
REQUESTED_ARG=""
EXTRA_BUILD_ARGS=""
MAKE_CORES=1

function print_help {
	echo "Usage: ethbuild.sh [extra-options]"
	echo "Arguments:"
	echo "    --help                  Print this help message."
	echo "    --branch NAME           The branch requested to build. Default is ${REQUESTED_BRANCH}."
	echo "    --clean-build           If given the build directories will be cleared."
	echo "    --build-type BUILDTYPE  If given then this is gonna be the value of -DCMAKE_BUILD_TYPE. Default is ${BUILD_TYPE} "
	echo "   --cores NUMBER           the value to the cores argument of make. e.g.: make -j4. Default is ${MAKE_CORES}."
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

	if [[ $arg == "--build-type" ]]; then
		REQUESTED_ARG="build-type"
		continue
	fi

	if [[ $arg == "--branch" ]]; then
		REQUESTED_ARG="build-type"
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

for project in "${PROJECTS[@]}"
do
	cd $project >/dev/null 2>/dev/null
	if [[ $? -ne 0 ]]; then
		echo "Skipping ${project} because directory does not exit";
		# Go back to root directory
		cd $ROOT_DIR
		continue
	fi
	BRANCH="$(git symbolic-ref HEAD 2>/dev/null)" ||
	BRANCH="(unnamed branch)"	  # detached HEAD
	BRANCH=${BRANCH##refs/heads/}

	if [[ $BRANCH != $REQUESTED_BRANCH ]]; then
		echo "BUILD WARNING: ${project} was in ${BRANCH} branch, while building on ${REQUESTED_BRANCH} was requested.";
		git checkout $REQUESTED_BRANCH
		if [[ $? -ne 0 ]]; then
			echo "ERROR: Checking out branch ${REQUESTED_BRANCH} for ${project} failed";
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

	# Go in build directory for the project and configure
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE $EXTRA_BUILD_ARGS;
	if [[ $? -ne 0 ]]; then
		echo "ERROR: cmake configure phase for project \"$project\" failed.";
		exit 1
	fi

	# Build the project
	make -j${MAKE_CORES}
	if [[ $? -ne 0 ]]; then
		echo "ERROR: Building project \"$project\" failed.";
		exit 1
	fi
	# Go back to root directory
	cd $ROOT_DIR
done
