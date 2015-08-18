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

set(CMAKE_AUTOMOC ON)

set(BUILD_DIR_NAME "build" CACHE STRING "The name of the build directory in cpp-ethereum")
set(CPPETHEREUM_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum/${BUILD_DIR_NAME}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPETHEREUM_BUILD})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum/cmake)
set(JSONRPC 1)
add_definitions(-DETH_TRUE)
include(EthDependencies)
include(EthExecutableHelper)
include(EthCompilerSettings)

set(P ${CPPETHEREUM_BUILD}/)
set(LIBS whisper;ethereum;evm;ethcore;lll;p2p;evmasm;devcrypto;evmcore;natspec;devcore;ethash-cl;ethash;secp256k1;scrypt;jsqrc)
set(CMAKE_LIBRARY_PATH ${P}secp256k1;${P}evmjit;${P}libsolidity;${CMAKE_LIBRARY_PATH})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cpp-ethereum)
include_directories(${CPPETHEREUM_BUILD})

find_library(SOLIDITY_LIBRARY NAMES solidity)
if (SOLIDITY_LIBRARY)
	add_definitions(-DETH_SOLIDITY)
endif()

find_library(EVMJIT_LIBRARY NAMES evmjit)
if (EVMJIT_LIBRARY)
	add_definitions(-DETH_EVMJIT)
endif()
foreach (l ${LIBS})
	set(CMAKE_LIBRARY_PATH ${P}lib${l};${CMAKE_LIBRARY_PATH})
	find_library(${l}_LIBRARY NAMES ${l})
endforeach()

add_subdirectory(src)

