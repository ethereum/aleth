cmake_minimum_required(VERSION 3.0.0)

# A place where should we look for *.cmake files
set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/../webthree-helpers/cmake"   CACHE PATH "The the path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# Set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(alethzero VERSION "1.2.3")

# Let's find our dependencies
include(EthDependencies)

# Figure out what compiler and system are we using
include(EthCompilerSettings)

# Include helper macros
include(EthExecutableHelper)

# Include utils
include(EthUtils)

include(EthOptions)
configure_project(SOLIDITY)

# required for qt5_wrap_ui below, TODO: remove this
find_package(Qt5Widgets)

add_subdirectory(libaleth)
add_subdirectory(alethzero)
