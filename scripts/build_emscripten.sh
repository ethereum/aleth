#!/usr/bin/env bash
#
# This script builds the solidity binary using emscripten.
# Emscripten is a way to compile C/C++ to JavaScript.
#
# This build script is supposed to be run on ubuntu, it might fail
# in other systems (pull requests appreciated!)
#
# Run this script inside a checkout of webthree-umbrella, i.e.
# git clone --recursive -b develop https://github.com/ethereum/webthree-umbrella
# It will create files and directories for the build tools that should
# not end up as part of webthree-umbrella!

# Multiple invocations of this script will try to download and build the
# build tools and dependencies only once.

# The path to the current directory should not contain spaces.

# The resulting "binary" will appear as
# soljson-$VER-$DATE-$COMMIT.js"

set -e

if [ -z ${WORKSPACE} ]
then
	WORKSPACE=$(pwd)
fi
export WORKSPACE
export PATH=$PATH:${WORKSPACE}/emsdk_portable/clang/master/build_master/bin:${WORKSPACE}/emsdk_portable/emscripten/master
export EMSCRIPTEN=${WORKSPACE}/emsdk_portable/emscripten/master

# Only install emscripten if it is not available yet.
cd "$WORKSPACE"
em++ --version || (

wget  -c https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz
tar xzf emsdk-portable.tar.gz

cd emsdk_portable
./emsdk update && ./emsdk install latest && ./emsdk activate latest
cd emscripten
ln -sf $(ls -d tag-?.??.?? | tail -n 1) master
cd ../clang
ln -sf  $(ls -d tag-??.??.?? | tail -n 1) master
cd master
ln -sf $(ls -d build_tag-* | tail -n 1) build_master
# We need to rename node to nodejs on debian-based distributions
sed -i "s/NODE_JS = 'node'/NODE_JS = 'nodejs'/g" ~/.emscripten 
# Reduce some code bloat
find "$WORKSPACE"/emsdk_portable -name '*.a' -o -name '*.o' -exec rm {} \;
find "$WORKSPACE"/emsdk_portable | xargs strip
)



# CryptoPP
cd "$WORKSPACE"
[ -x cryptopp ] || git clone https://github.com/mmoss/cryptopp.git
cd cryptopp
emcmake cmake -DCRYPTOPP_LIBRARY_TYPE=STATIC -DCRYPTOPP_RUNTIME_TYPE=STATIC && emmake make -j 4
ln -s . src/cryptopp || true




# Json-CPP
cd "$WORKSPACE"
[ -x jsoncpp ] || git clone https://github.com/open-source-parsers/jsoncpp.git
cd jsoncpp
emcmake cmake -DJSONCPP_LIB_BUILD_STATIC=ON -DJSONCPP_LIB_BUILD_SHARED=OFF \
              -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
              -G "Unix Makefiles" .
emmake make -j 4



# Boost
cd "$WORKSPACE"
[ -x boost_1_57_0 ] || (
wget 'http://downloads.sourceforge.net/project/boost/boost/'\
'1.57.0/boost_1_57_0.tar.bz2?r=http%3A%2F%2Fsourceforge.net%2F'\
'projects%2Fboost%2Ffiles%2Fboost%2F1.57.0%2F&ts=1421887207'\
 -O boost_1_57_0.tar.bz2
tar xjf boost_1_57_0.tar.bz2
)

cd "$WORKSPACE"/boost_1_57_0
[ -x libboost_system.a ] || (
./bootstrap.sh --with-libraries=thread,system,regex,date_time,chrono,filesystem,program_options,random
sed -i 's|using gcc ;|using gcc : : '"$WORKSPACE"'/emsdk_portable/emscripten/master/em++ ;|g' ./project-config.jam
sed -i 's|$(archiver\[1\])|'"$WORKSPACE"'/emsdk_portable/emscripten/master/emar|g' ./tools/build/src/tools/gcc.jam
sed -i 's|$(ranlib\[1\])|'"$WORKSPACE"'/emsdk_portable/emscripten/master/emranlib|g' ./tools/build/src/tools/gcc.jam
./b2 link=static variant=release threading=single runtime-link=static \
       thread system regex date_time chrono filesystem unit_test_framework program_options random
find . -name 'libboost*.a' -exec cp {} . \;
)



for component in webthree-helpers libweb3core libethereum solidity
do
  cd "$WORKSPACE"
  if [ -x $component ]
  then
    cd $component
    git fetch origin
    git checkout origin/develop
  else
    git clone https://github.com/ethereum/$component
  fi
  if [ "$component" = solidity -a -n "$SolidityBranch" ]
  then
    git fetch $SolidityBranch
    git checkout FETCH_HEAD
  fi
done

# Build dependent components and solidity itself
for component in webthree-helpers/utils libweb3core libethereum solidity
do
cd "$WORKSPACE/$component"
mkdir build || true
cd build
emcmake cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DEMSCRIPTEN=1 \
  -DCMAKE_CXX_COMPILER=em++ \
  -DCMAKE_C_COMPILER=emcc \
  -DBoost_FOUND=1 \
  -DBoost_USE_STATIC_LIBS=1 \
  -DBoost_USE_STATIC_RUNTIME=1 \
  -DBoost_INCLUDE_DIR="$WORKSPACE"/boost_1_57_0/ \
  -DBoost_CHRONO_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_chrono.a \
  -DBoost_CHRONO_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_chrono.a \
  -DBoost_DATE_TIME_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_date_time.a \
  -DBoost_DATE_TIME_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_date_time.a \
  -DBoost_FILESYSTEM_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_filesystem.a \
  -DBoost_FILESYSTEM_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_filesystem.a \
  -DBoost_PROGRAM_OPTIONS_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_program_options.a \
  -DBoost_PROGRAM_OPTIONS_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_program_options.a \
  -DBoost_RANDOM_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_random.a \
  -DBoost_RANDOM_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_random.a \
  -DBoost_REGEX_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_regex.a \
  -DBoost_REGEX_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_regex.a \
  -DBoost_SYSTEM_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_system.a \
  -DBoost_SYSTEM_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_system.a \
  -DBoost_THREAD_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_thread.a \
  -DBoost_THREAD_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_thread.a \
  -DBoost_UNIT_TEST_FRAMEWORK_LIBRARY="$WORKSPACE"/boost_1_57_0/libboost_unit_test_framework.a \
  -DBoost_UNIT_TEST_FRAMEWORK_LIBRARIES="$WORKSPACE"/boost_1_57_0/libboost_unit_test_framework.a \
  -DJSONCPP_LIBRARY="$WORKSPACE"/jsoncpp/src/lib_json/libjsoncpp.a \
  -DJSONCPP_INCLUDE_DIR="$WORKSPACE"/jsoncpp/include/ \
  -DCRYPTOPP_LIBRARY="$WORKSPACE"/cryptopp/src/libcryptlib.a \
  -DCRYPTOPP_INCLUDE_DIR="$WORKSPACE"/cryptopp/src/ \
  -DDev_DEVCORE_LIBRARY="$WORKSPACE"/libweb3core/build/libdevcore/libdevcore.a \
  -DDev_DEVCRYPTO_LIBRARY="$WORKSPACE"/libweb3core/build/libdevcrypto/libdevcrypto.a \
  -DEth_EVMCORE_LIBRARY="$WORKSPACE"/libethereum/build/libevmcore/libevmcore.a \
  -DEth_EVMASM_LIBRARY="$WORKSPACE"/solidity/build/libevmasm/libevmasm.a \
  -DUtils_SCRYPT_LIBRARY="$WORKSPACE"/webthree-helpers/utils/build/libscrypt/libscrypt.a \
  -DETHASHCL=0 -DEVMJIT=0 -DETH_STATIC=1 -DSOLIDITY=1 -DGUI=0 -DFATDB=0 -DTESTS=0 -DTOOLS=0 \
  ..
emmake make -j 4
done


# Extract the new binary
if [ -z "$SolidityBranch" ]
then
  cd "$WORKSPACE"/solidity
  VER=$(git describe --tag --abbrev=0)
  COMMIT=$(git rev-parse --short HEAD)
  DATE=$(date --date="$(git log -1 --date=iso --format=%ad HEAD)" --utc +%F)
  mv build/solc/soljson.js "../soljson-$VER-$DATE-$COMMIT.js"
fi
