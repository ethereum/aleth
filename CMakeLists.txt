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

cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

set(CPP_ETHEREUM_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Path to the root directory for cpp-ethereum")

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(cpp-ethereum VERSION "1.3.0")

include(EthCompilerSettings)
include(EthExecutableHelper)
include(EthDependencies)
include(EthUtils)
include(EthOptions)

include(cmake/jsoncpp.cmake)
include(cmake/jsonrpc.cmake)

configure_project(CPUID CURL ETHASHCL EVMJIT FATDB MINIUPNPC ROCKSDB PARANOID TESTS VMTRACE)

add_subdirectory(eth)
add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libethash)
add_subdirectory(libethashseal)
add_subdirectory(libethcore)
add_subdirectory(libethereum)
add_subdirectory(libevm)
add_subdirectory(libevmcore)
if (NOT EMSCRIPTEN)
    add_subdirectory(libp2p)
endif()
add_subdirectory(libweb3jsonrpc)
add_subdirectory(libwebthree)
add_subdirectory(libwhisper)

if (ETHASHCL)
    add_subdirectory(libethash-cl)
endif()

if (EVMJIT)
    add_subdirectory(evmjit)
endif()

if (TOOLS)
    add_subdirectory(ethkey)
    add_subdirectory(ethminer)
    add_subdirectory(ethvm)
    if (NOT EMSCRIPTEN)
        add_subdirectory(rlp)
        add_subdirectory(bench)
    endif()
endif()

if (TESTS)
	add_subdirectory(test)
endif()

# TODO - split out json_spirit, libscrypt and sec256k1
add_subdirectory(utils)
