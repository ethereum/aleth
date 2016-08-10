
cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

set(UTILS_DIR "${CMAKE_CURRENT_LIST_DIR}/utils" CACHE PATH "The path to the utils libraries directory")
set(DEV_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "The path to the dev libraries directory")
set(ETH_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "The path to the eth libraries directory")
set(WEB3_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "The path to the webthree libraries directory")
#set(SOL_DIR "${CMAKE_CURRENT_LIST_DIR}/solidity" CACHE PATH "The path to the solidity libraries directory")

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
configure_project(CPUID CURL ETHASHCL EVMJIT FATDB GUI MINIUPNPC OLYMPIC ROCKSDB PARANOID SOLIDITY TESTS VMTRACE)

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
    add_subdirectory(test/libethereum/test)
    add_subdirectory(test/libweb3core/test)
    add_subdirectory(test/webthree/test)
endif()

# TODO - split out json_spirit, libscrypt and sec256k1
add_subdirectory(utils)
