#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to build the binaries of ethereum cpp repos
# in jenkins. Uses the umbrella repo.

function print_help {
	echo "Usage: ethbinaries.sh [options]"
	echo "Arguments:"
	echo "    --help                  Print this help message."
	echo "    --keep-build            If given the build directories will not be cleared."
	echo "    --cores NUMBER          The value to the cores argument of make. e.g.: make -j4. Default is ${MAKE_CORES}."
	echo "    --version VERSION       A string to append to the binary for the version."
}

CLEAN_BUILD=1
MAKE_CORES=4
GIVEN_VERSION=1.0rc2 #default - mainly for testing if no version is given

for arg in ${@:1}
do
	if [[ ${REQUESTED_ARG} != "" ]]; then
	case $REQUESTED_ARG in
		"make-cores")
			re='^[0-9]+$'
			if ! [[ $arg =~ $re ]] ; then
				echo "ERROR: Value for --cores is not a number!"
				exit 1
			fi
			MAKE_CORES=$arg
			;;
		"version")
			GIVEN_VERSION=$arg
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

	if [[ $arg == "--keep-build" ]]; then
		CLEAN_BUILD=0
		continue
	fi

	if [[ $arg == "--cores" ]]; then
		REQUESTED_ARG="make-cores"
		continue
	fi

	if [[ $arg == "--version" ]]; then
		REQUESTED_ARG="version"
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

if [[ ! -d "webthree-umbrella" ]]; then
	git clone --recursive https://github.com/ethereum/webthree-umbrella
fi
cd webthree-umbrella

# Make/clean build directory depending on requested arguments
if [[ -d "build" ]]; then
	if [[ $CLEAN_BUILD -eq 1 ]]; then
		rm -rf build
		mkdir build
	fi
else
	mkdir build
fi
cd build

if [[ $OSTYPE == "cygwin" ]]; then
	# we should be in webthree-umbrella/build
	source ../webthree-helpers/scripts/ethwindowsenv.sh
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Failed to source ethwindowsenv.sh.";
		exit 1
	fi
	cmake .. -G "Visual Studio 12 2013 Win64"
	if [[ $? -ne 0 ]]; then
	echo "ETHBINARIES - ERROR: cmake configure phase in Windows failed.";
	exit 1
	fi
	# "${MSBUILD_EXECUTABLE}" cpp-ethereum.sln /p:Configuration=Release /t:PACKAGE /m:${MAKE_CORES}
	cmake --build . --target package -- /m:${MAKE_CORES} /p:Configuration=Release
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Building binaries for Windows failed.";
		exit 1
	fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
	cmake .. -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=RelWithDebInfo
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: cmake configure phase failed.";
		exit 1
	fi
	make -j${MAKE_CORES} appdmg
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Building a DMG for Macosx failed.";
		exit 1
	fi
	make -j${MAKE_CORES} install
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Make install for Macosx failed.";
		exit 1
	fi
	../webthree-helpers/homebrew/prepare_receipt.sh --version $GIVEN_VERSION --number ${BUILD_NUMBER}
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Make install for Macosx failed.";
		exit 1
	fi
else
	echo "ETHBINARIES - WARNING: Requested to build unneeded platform. Ignoring ...";
fi

