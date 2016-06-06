cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(utils VERSION "1.2.7")

include(EthCompilerSettings)

include(EthExecutableHelper)

include(EthDependencies)

add_subdirectory(libscrypt)

if (NOT EMSCRIPTEN)
	add_subdirectory(secp256k1)
endif()
