#------------------------------------------------------------------------------
# Top-level CMake file for cpp-ethereum.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2014-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.4.3)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # The default toolchain file configures compilers and build environment.
    # This configuration is also used by hunter to build dependencies.
    # CMake will cache this value, not need to explictly specify CACHE param.
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)
endif()

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

set(CPP_ETHEREUM_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Path to the root directory for cpp-ethereum")

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(cpp-ethereum VERSION "1.3.0")

if (NOT EXISTS ${CMAKE_SOURCE_DIR}/evmjit/.git)
    message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init")
endif()

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY On)

include(EthOptions)
include(EthCompilerSettings)
include(EthExecutableHelper)
include(EthDependencies)
include(EthUtils)

include(ProjectBoost)
include(ProjectCryptopp)
include(ProjectJsonCpp)
include(ProjectJsonRpcCpp)
include(ProjectSecp256k1)
include(deps/snark.cmake)

configure_project()

add_subdirectory(eth)
add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libethash)
add_subdirectory(libethashseal)
add_subdirectory(libethcore)
add_subdirectory(libethereum)
add_subdirectory(libevm)
add_subdirectory(libevmcore)
add_subdirectory(libp2p)
add_subdirectory(libweb3jsonrpc)
add_subdirectory(libwebthree)
add_subdirectory(libwhisper)

if (EVMJIT)
    add_subdirectory(evmjit)
endif()

if (TOOLS)
    add_subdirectory(ethkey)
    add_subdirectory(ethvm)
    add_subdirectory(rlp)
    add_subdirectory(bench)
endif()

if (TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

# TODO: Split out json_spirit, libscrypt and sec256k1.
add_subdirectory(utils)
