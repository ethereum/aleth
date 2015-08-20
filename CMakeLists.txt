cmake_minimum_required(VERSION 2.8.12)

cmake_policy(SET CMP0015 NEW)
# let cmake autolink dependencies on windows
cmake_policy(SET CMP0020 NEW)
# this policy was introduced in cmake 3.0
# remove if, once 3.0 will be used on unix
if (${CMAKE_MAJOR_VERSION} GREATER 2)
	cmake_policy(SET CMP0043 OLD)
endif()

set(PROJECT_VERSION "0.9.42")
if (${CMAKE_VERSION} VERSION_GREATER 3.0)
	cmake_policy(SET CMP0042 OLD) 	# fix MACOSX_RPATH
	cmake_policy(SET CMP0048 NEW) 	# allow VERSION argument in project()
	project(webthree VERSION ${PROJECT_VERSION})
else()
	project(webthree)
endif()

# Figure out environment.
set(BUILD_DIR_NAME "build" CACHE STRING "The name of the build directory in cpp-ethereum")
set(CPPETHEREUM_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum/${BUILD_DIR_NAME}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPETHEREUM_BUILD})
message("CPPETHEREUM_BUILD: ${CPPETHEREUM_BUILD}")
set(ETH_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum-cmake")

list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

set(GUI 1)		# TODO: remove once packages are all changed to optional
set(JSONRPC 1)	# TODO: remove once packages are all changed to optional

include(EthDependencies)
include(EthExecutableHelper)
include(EthCompilerSettings)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../solidity)
set(CMAKE_THREAD_LIBS_INIT pthread)

set(EVMJIT 0)
set(CMAKE_LIBRARY_PATH ${CPPETHEREUM_BUILD};${CMAKE_LIBRARY_PATH})
set(LIBS whisper;ethereum;evm;ethcore;lll;p2p;evmasm;devcrypto;evmcore;natspec;devcore;ethash-cl;ethash;secp256k1;scrypt;jsqrc)
find_library(devcore_LIBRARY NAMES devcore PATH_SUFFIXES libdevcore)
find_library(devcrypto_LIBRARY NAMES devcrypto PATH_SUFFIXES libdevcrypto)
find_library(ethcore_LIBRARY NAMES ethcore PATH_SUFFIXES libethcore)
find_library(ethereum_LIBRARY NAMES ethereum PATH_SUFFIXES libethereum)
find_library(evm_LIBRARY NAMES evm PATH_SUFFIXES libevm)
find_library(p2p_LIBRARY NAMES p2p PATH_SUFFIXES libp2p)
find_library(whisper_LIBRARY NAMES whisper PATH_SUFFIXES libwhisper)
find_library(lll_LIBRARY NAMES lll PATH_SUFFIXES liblll)
find_library(solidity_LIBRARY NAMES solidity)
if (solidity_LIBRARY)
	set(SOLIDITY 1)
endif()
find_library(serpent_LIBRARY NAMES serpent)
if (serpent_LIBRARY)
	set(SERPENT 1)
endif()
find_library(evmjit_LIBRARY NAMES evmjit)
if (evmjit_LIBRARY)
	set(EVMJIT 1)
endif()
find_library(jsconsole_LIBRARY NAMES jsconsole)
if (jsconsole_LIBRARY)
	set(JSCONSOLE 1)
	set(LIBS "${LIBS};jsengine")
endif()
foreach (l ${LIBS})
	find_library(${l}_LIBRARY NAMES ${l})
endforeach()

add_subdirectory(src)

