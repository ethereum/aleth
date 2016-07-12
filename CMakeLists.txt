cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../webthree-helpers/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(ethereum VERSION "1.2.9")

include(EthCompilerSettings)

include(EthExecutableHelper)

include(EthDependencies)

include(EthUtils)

include(EthOptions)

set(LIBETHEREUM_OPTIONS CPUID CURL VMTRACE ROCKSDB OLYMPIC PARANOID TESTS ETHASHCL FATDB)
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|AMD64")
	# EVMJIT option is only available for x86_64 targets.
	list(APPEND LIBETHEREUM_OPTIONS EVMJIT)
else()
	set(EVMJIT OFF)
endif()
configure_project(${LIBETHEREUM_OPTIONS})

if (EVMJIT)
	if (DEFINED MSVC AND NOT DEFINED LLVM_DIR)
		set(LLVM_DIR "${ETH_CMAKE_DIR}/extdep/install/windows/x64/share/llvm/cmake")
	endif()
	add_subdirectory(evmjit)
	if (DEFINED MSVC)
		set(EVMJIT_DLLS_LOCAL $<TARGET_FILE:evmjit>)
		set(EVMJIT_DLLS optimized ${EVMJIT_DLLS_LOCAL} debug ${EVMJIT_DLLS_LOCAL} PARENT_SCOPE)
	endif()
endif()

# core libraries
if (NOT EMSCRIPTEN)
	add_subdirectory(libethcore)
	add_subdirectory(libevm)
	add_subdirectory(libethereum)

	add_subdirectory(libethash)
	add_subdirectory(libethashseal)
endif()
add_subdirectory(libevmcore)

if (ETHASHCL)
	add_subdirectory(libethash-cl)
endif()

# executables
if (TOOLS)
	add_subdirectory(ethkey)
	add_subdirectory(ethminer)
	add_subdirectory(ethvm)
endif()

if (TESTS)
	# test
	add_subdirectory(test)
endif()
