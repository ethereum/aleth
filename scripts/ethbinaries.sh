#!/usr/bin/env bash
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
	echo "    --devtest               Don't build binaries but instead just try to build and run the tests."
}

DEV_TEST=0
CLEAN_BUILD=1
MAKE_CORES=4
GIVEN_VERSION="" # Will be extracted from CMakeLists if not explicitly given

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

	if [[ $arg == "--devtest" ]]; then
		DEV_TEST=1
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

# If we need to also run tests clone/update the tests repo
if [[ $DEV_TEST -eq 1 ]]; then
	if [[ ! -d "tests" ]]; then
		git clone https://github.com/ethereum/tests
	else
		cd tests
		git pull origin master
		cd ..
	fi
fi

if [[ ! -d "webthree-umbrella" ]]; then
	git clone --recursive https://github.com/ethereum/webthree-umbrella
fi
cd webthree-umbrella

if [ -z "$GIVEN_VERSION" ]
then
	GIVEN_VERSION=$(grep -e "[_ ]VERSION \"" CMakeLists.txt | sed -e 's/.*VERSION "\([^"]*\)".*/\1/')
	echo "ETHBINARIES - found version ${GIVEN_VERSION}"
fi

# Make/clean build directory depending on requested arguments
if [[ -d "build" ]]; then
	if [[ $CLEAN_BUILD -eq 1 ]]; then
		rm -rf build
		mkdir build
	else
		 # Delete all previous binaries in the workspace
		 rm -rf build/*.exe
		 rm -rf build/*.dmg
		 rm -rf build/cpp-ethereum.rb
		 rm -rf build/*.tar.gz
	fi
else
	mkdir build
fi
cd build

if [[ $OSTYPE == "cygwin" ]]; then
	echo "ETHBINARIES - INFO: Building Windows binaries.";
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
	cmake --build . --target package -- /m:${MAKE_CORES} /p:Configuration=RelWithDebInfo
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Building binaries for Windows failed.";
		exit 1
	fi
elif [[ "$OSTYPE" == "darwin"* ]]; then

	# Detect whether we are running on a Yosemite or El Capitan machine.
	if echo `sw_vers` | grep "10.11"; then
		OSX_VERSION=elcapitan
	elif echo `sw_vers` | grep "10.10"; then
		OSX_VERSION=yosemite
	else
		echo Unsupported OS X version.  We only support Yosemite and El Capitan
		exit 1
	fi

	if [[ $DEV_TEST -eq 1 ]]; then
		echo "ETHBINARIES - INFO: Building MacOSX for development test.";
		cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
		make -j4
		if [[ $? -ne 0 ]]; then
			echo "ETHBINARIES - ERROR: Building for Macosx failed.";
			exit 1
		fi
	else
		echo "ETHBINARIES - INFO: Building MacOSX binaries.";
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
		mv cpp-ethereum-osx.dmg cpp-ethereum-osx-${OSX_VERSION}.dmg
		
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
		# create eth binaries zip
		rm -rf ethbin
		mkdir ethbin
		cp ./install/lib/*.dylib ethbin/
		cp ./install/bin/eth ethbin/
		cd ethbin
		/usr/bin/env ruby ../../webthree-helpers/scripts/locdep.rb eth .
		for f in *.dylib ; do /usr/bin/env ruby ../../webthree-helpers/scripts/locdep.rb $f . ; done
		# run again to process dylibs copied on previous step
		for f in *.dylib ; do /usr/bin/env ruby ../../webthree-helpers/scripts/locdep.rb $f . ; done
		cd ..
		zip cpp-ethereum-osx-${OSX_VERSION}.zip ethbin/*

	fi
else
	echo "ETHBINARIES - INFO: Building for Linux ...";
	cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make -j4
	if [[ $? -ne 0 ]]; then
		echo "ETHBINARIES - ERROR: Make  for Linux failed.";
		exit 1
	fi
fi

if [[ $OSTYPE != "cygwin" && $DEV_TEST -eq 1 ]]; then
	echo "ETHBINARIES - INFO: Running tests ...";
	# run all tests
	cd ..
	webthree-helpers/scripts/ethtests.sh libweb3core --umbrella
fi
