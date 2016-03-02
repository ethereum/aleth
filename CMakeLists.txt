cmake_minimum_required(VERSION 2.8.12)
# A place where should we look for *.cmake files
set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/../webthree-helpers/cmake"   CACHE PATH "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})
set(JSCONSOLE 1)

# Set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(webthree VERSION "1.2.1")

# Let's find our dependencies
include(EthDependencies)

# Figure out what compiler and system are we using
include(EthCompilerSettings)

# Include helper macros
include(EthExecutableHelper)

# Include utils
include(EthUtils)

include(EthOptions)
configure_project(TESTS)

# libraries
add_subdirectory(libwebthree)
add_subdirectory(libweb3jsonrpc)
add_subdirectory(libwhisper)

# other libs
add_subdirectory(libjsengine)
add_subdirectory(libjsconsole)

# executables
add_subdirectory(eth)

# test stuff
add_subdirectory(test)
