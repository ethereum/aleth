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

cmake_minimum_required(VERSION 3.5.1)

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

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
endif()

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

set(HUNTER_CONFIGURATION_TYPES Release)
set(HUNTER_JOBS_NUMBER 4)
set(HUNTER_CACHE_SERVERS "https://github.com/ethereum/hunter-cache")
include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.19.173.tar.gz"
    SHA1 "750fb1d621af971fad6f303356b13017ad9f5e15"
    LOCAL
)

if(HUNTER_ENABLED)
    find_package(PythonInterp QUIET)
    if(PYTHONINTERP_FOUND)
        hunter_gate_self(
            "${HUNTER_CACHED_ROOT}"
            "${HUNTER_VERSION}"
            "${HUNTER_SHA1}"
            hunter_dir
        )
        set(hunter_upload_script "${hunter_dir}/maintenance/upload-cache-to-github.py")
        set(hunter_cache_dir "${HUNTER_GATE_ROOT}/_Base/Cache")
        set(hunter_tmp_dir "${HUNTER_GATE_ROOT}/tmp")
        add_custom_target(
            hunter_upload_cache
            ${PYTHON_EXECUTABLE} ${hunter_upload_script}
            --username hunter-cache-bot
            --repo-owner ethereum
            --repo hunter-cache
            --cache-dir ${hunter_cache_dir}
            --temp-dir ${hunter_tmp_dir}
        )
    endif()
endif()

# project name and version should be set after cmake_policy CMP0048
project(cpp-ethereum VERSION "1.3.0")

if (NOT EXISTS ${CMAKE_SOURCE_DIR}/evmjit/.git)
    message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init")
endif()

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY On)

include(EthCcache)
include(EthOptions)
include(EthCompilerSettings)
include(EthExecutableHelper)
include(EthDependencies)
include(EthUtils)

hunter_add_package(Boost COMPONENTS program_options filesystem system thread)
find_package(Boost CONFIG REQUIRED program_options filesystem system thread)

hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

hunter_add_package(Snappy)
find_package(Snappy CONFIG REQUIRED)

hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)

include(ProjectJsonRpcCpp)
include(ProjectSecp256k1)
include(ProjectLibFF)

find_package(Threads)

if(MINIUPNPC)
    find_package(Miniupnpc 1.8.2013 REQUIRED)
endif()

set(UTILS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/utils")

configure_project()

#Global include path for all libs.
include_directories("${CMAKE_SOURCE_DIR}")

add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)

add_subdirectory(libethash)

add_subdirectory(libethcore)
add_subdirectory(libevm)
add_subdirectory(libethereum)
add_subdirectory(libethashseal)

add_subdirectory(libwebthree)
add_subdirectory(libweb3jsonrpc)

if (EVMJIT)
    add_subdirectory(evmjit)
endif()

add_subdirectory(eth)

if (TOOLS)
    add_subdirectory(ethkey)
    add_subdirectory(ethvm)
    add_subdirectory(rlp)
endif()

if (TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# TODO: Split out json_spirit, libscrypt and sec256k1.
add_subdirectory(utils)
