cmake_minimum_required(VERSION 2.8.12)
# A place where should we look for *.cmake files
set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/../cpp-ethereum-cmake"   CACHE PATH "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# Set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
set(PROJECT_VERSION "0.9.42")
project(webthree VERSION ${PROJECT_VERSION})

# Let's find our dependencies
include(EthDependencies)

# Figure out what compiler and system are we using
include(EthCompilerSettings)

# Include helper macros
include(EthExecutableHelper)

# Include utils
include(EthUtils)

add_subdirectory(src)

