cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../webthree-helpers/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(ethereum VERSION "0.9.92")

include(EthCompilerSettings)

include(EthExecutableHelper)

include(EthDependencies)

include(EthUtils)

include(EthOptions)
configure_project(CPUID CURL VMTRACE ROCKSDB OLYMPIC PARANOID TESTS ETHASHCL EVMJIT FATDB)

if (EVMJIT)
	if (NOT DEFINED LLVM_DIR)
		if (DEFINED MSVC)
			set(LLVM_DIR "${ETH_CMAKE_DIR}/extdep/install/windows/x64/share/llvm/cmake")
		elseif (DEFINED APPLE)
			set(LLVM_DIR "/usr/local/opt/llvm/share/llvm/cmake")
		endif()
	endif()

	add_subdirectory(evmjit)
	if (DEFINED MSVC)
		set(EVMJIT_DLLS_LOCAL $<TARGET_FILE:evmjit>)
		set(EVMJIT_DLLS optimized ${EVMJIT_DLLS_LOCAL} debug ${EVMJIT_DLLS_LOCAL} PARENT_SCOPE)
	endif()
endif()

# core libraries
if (NOT EMSCRIPTEN)
	add_subdirectory(libethash)
	add_subdirectory(libethcore)
	add_subdirectory(liblll)
	add_subdirectory(libevm)
	add_subdirectory(libethereum)
endif()
add_subdirectory(libevmcore)
add_subdirectory(libevmasm)

if (ETHASHCL)
	add_subdirectory(libethash-cl)
endif()

# other libraries
add_subdirectory(libnatspec)
if (NOT EMSCRIPTEN)
	add_subdirectory(libtestutils)
endif()

# executables

if (TOOLS)
	add_subdirectory(ethkey)
	add_subdirectory(ethminer)
	add_subdirectory(ethvm)
	add_subdirectory(lllc)
endif()

if (TESTS)
	# test
	add_subdirectory(test)
endif()

