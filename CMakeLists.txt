# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

cmake_minimum_required(VERSION 3.5.1)

include(evmc/cmake/cable/bootstrap.cmake)
include(CableBuildInfo)
include(CableBuildType)
include(CableToolchains)
include(GNUInstallDirs)

if(UNIX)
    # Build deps with PIC to allow building shared libraries.
    set(toolchain cxx11-pic)
else()
    set(toolchain cxx11)
endif()
cable_configure_toolchain(DEFAULT ${toolchain})

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

set(HUNTER_CONFIGURATION_TYPES Release CACHE STRING "Hunter configuration types")
set(HUNTER_JOBS_NUMBER 4 CACHE STRING "Hunter jobs number")
set(HUNTER_CACHE_SERVERS "https://github.com/ethereum/hunter-cache" CACHE STRING "Hunter cache servers")
set(HUNTER_PASSWORDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Hunter/passwords.cmake CACHE STRING "Hunter password path")

# In CI builds upload the binaries if the GITHUB_USER_PASSWORD was decrypted
# (only for branches and internal PRs).
if("$ENV{CI}" AND NOT "$ENV{GITHUB_USER_PASSWORD}" STREQUAL "")
    set(run_upload YES)
else()
    set(run_upload NO)
endif()
option(HUNTER_RUN_UPLOAD "Upload binaries to the cache server" ${run_upload})

include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.21.16.tar.gz"
    SHA1 "aa1faccd24f33dd2c368f85a4e9ca2e5d9c249cb"
    LOCAL
)

project(aleth)
set(PROJECT_VERSION 1.5.0.dev0)

if (NOT EXISTS ${CMAKE_SOURCE_DIR}/evmjit/.git)
    message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init")
endif()

cable_set_build_type(DEFAULT RelWithDebInfo CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)

cable_add_buildinfo_library(PROJECT_NAME aleth)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY On)

include(EthCcache)
include(EthOptions)
include(EthCompilerSettings)
include(EthExecutableHelper)
include(EthDependencies)
include(EthUtils)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
hunter_add_package(Boost COMPONENTS program_options filesystem system thread context fiber log)
find_package(Boost CONFIG REQUIRED program_options filesystem system thread context fiber log)

hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

hunter_add_package(Snappy)
find_package(Snappy CONFIG REQUIRED)

hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)

hunter_add_package(libjson-rpc-cpp)
find_package(libjson-rpc-cpp CONFIG REQUIRED)

hunter_add_package(libscrypt)
find_package(libscrypt CONFIG REQUIRED)

hunter_add_package(ethash)
find_package(ethash CONFIG REQUIRED)

include(ProjectSecp256k1)
include(ProjectLibFF)

find_package(Threads)

if(MINIUPNPC)
    find_package(Miniupnpc 1.8.2013 REQUIRED)
endif()

set(UTILS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/utils")

configure_project()

if(TESTS)
    # Import binaryen library before hera to make it available also to testeth.
    include(hera/cmake/ProjectBinaryen.cmake)
endif()

add_subdirectory(evmc)

if (HERA)
    add_subdirectory(hera)
endif()

#Global include path for all libs.
include_directories("${CMAKE_SOURCE_DIR}")

option(ALETH_INTERPRETER_SHARED "Build aleth-interpreter as a shared library" OFF)
add_subdirectory(libaleth-interpreter)

add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)

add_subdirectory(libethcore)
add_subdirectory(libevm)
add_subdirectory(libethereum)
add_subdirectory(libethashseal)

add_subdirectory(libwebthree)
add_subdirectory(libweb3jsonrpc)

if (EVMJIT)
    add_subdirectory(evmjit)
endif()

add_subdirectory(aleth)

if (TOOLS)
    add_subdirectory(aleth-key)
    add_subdirectory(aleth-vm)
    add_subdirectory(rlp)
endif()

if (TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
