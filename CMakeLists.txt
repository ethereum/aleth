







# cmake global
cmake_minimum_required(VERSION 3.0.0)

# Figure out environment.
set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/../cpp-ethereum-cmake"   CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(ethereum VERSION "0.9.91")

set(CMAKE_AUTOMOC ON)

# note: The value "default" which provides the defaults is just a fake value
# which lets us keep the default values of all build options and is set at
# the beginning of this file.

# TODO: Abstract into something sensible and move into a function.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(DECENT_PLATFORM OFF)
else ()
	set(DECENT_PLATFORM ON)
endif ()

#defaults:
set(D_CMAKE_BUILD_TYPE "RelWithDebInfo")
set(D_SERPENT ${DECENT_PLATFORM})
set(D_USENPM OFF)
set(D_GUI ON)
set(D_TOOLS ON)
set(D_TESTS ON)
set(D_FATDB ON)
set(D_ETHASHCL ON)
set(D_EVMJIT ON)
set(D_JSCONSOLE ON)
set(D_VMTRACE OFF)
set(D_PARANOID OFF)
set(D_PROFILING OFF)
set(D_ROCKSDB OFF)
set(D_OLYMPIC OFF)
set(D_MINER ON)
set(D_ETHKEY ON)

if (BUNDLE STREQUAL "minimal")
	set(D_SERPENT OFF)
	set(D_USENPM OFF)
	set(D_GUI OFF)
	set(D_TOOLS ON)
	set(D_TESTS OFF)
elseif (BUNDLE STREQUAL "full")
	set(D_SERPENT ${DECENT_PLATFORM})
	set(D_USENPM ON)
	set(D_GUI ON)
	set(D_TOOLS ON)
	set(D_TESTS ON)
	set(D_FATDB ON)
elseif (BUNDLE STREQUAL "cli")
	set(D_SERPENT ${DECENT_PLATFORM})
	set(D_USENPM ON)
	set(D_GUI OFF)
	set(D_TOOLS ON)
	set(D_TESTS ON)
	set(D_FATDB ON)
elseif (BUNDLE STREQUAL "core")
	set(D_SERPENT OFF)
	set(D_USENPM OFF)
	set(D_GUI ON)
	set(D_TOOLS ON)
	set(D_TESTS OFF)
	set(D_FATDB ON)
elseif (BUNDLE STREQUAL "tests")
	set(D_SERPENT ${DECENT_PLATFORM})
	set(D_USENPM OFF)
	set(D_GUI OFF)
	set(D_TOOLS OFF)
	set(D_TESTS ON)
	set(D_FATDB ON)
elseif (BUNDLE STREQUAL "user")
	set(D_SERPENT OFF)
	set(D_USENPM OFF)
	set(D_GUI ON)
	set(D_TOOLS ON)
	set(D_TESTS OFF)
elseif (BUNDLE STREQUAL "wallet")
	set(D_SERPENT OFF)
	set(D_USENPM OFF)
	set(D_GUI OFF)
	set(D_TOOLS OFF)
	set(D_TESTS OFF)
	set(D_ETHKEY ON)
	set(D_MINER OFF)
	set(D_ETHASHCL OFF)
	set(D_FATDB OFF)
	set(D_JSCONSOLE OFF)
	set(D_EVMJIT OFF)
elseif (BUNDLE STREQUAL "miner")
	set(D_SERPENT OFF)
	set(D_USENPM OFF)
	set(D_GUI OFF)
	set(D_TOOLS OFF)
	set(D_TESTS OFF)
	set(D_ETHKEY OFF)
	set(D_MINER ON)
	set(D_ETHASHCL ON)
	set(D_FATDB OFF)
	set(D_JSCONSOLE OFF)
	set(D_EVMJIT OFF)
elseif (BUNDLE STREQUAL "release")      # release builds
	set(D_SERPENT ${DECENT_PLATFORM})
	set(D_USENPM OFF)
	set(D_GUI ON)
	set(D_TOOLS ON)
	set(D_TESTS OFF)
	set(D_FATDB OFF)
	set(D_ETHASHCL ON)
	set(D_EVMJIT ON)
	set(D_JSCONSOLE ON)
	set(D_CMAKE_BUILD_TYPE "Release")
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE ${D_CMAKE_BUILD_TYPE})
endif ()

# propagates CMake configuration options to the compiler
function(configureProject)
	if (PARANOID)
		add_definitions(-DETH_PARANOIA)
	endif ()

	if (VMTRACE)
		add_definitions(-DETH_VMTRACE)
	endif ()

	if (ETHASHCL)
		add_definitions(-DETH_ETHASHCL)
	endif()

	if (EVMJIT)
		add_definitions(-DETH_EVMJIT)
	endif()

	if (CPUID_FOUND)
		add_definitions(-DETH_CPUID)
	endif()

	if (CURL_FOUND)
		add_definitions(-DETH_CURL)
	endif()

	if (OLYMPIC)
		add_definitions(-DETH_OLYMPIC)
	else()
		add_definitions(-DETH_FRONTIER)
	endif()

	add_definitions(-DETH_TRUE)
endfunction()

set(CPPETHEREUM 1)

function(createBuildInfo)
	# Set build platform; to be written to BuildInfo.h
	set(ETH_BUILD_PLATFORM "${TARGET_PLATFORM}")
	if (CMAKE_COMPILER_IS_MINGW)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/mingw")
	elseif (CMAKE_COMPILER_IS_MSYS)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/msys")
	elseif (CMAKE_COMPILER_IS_GNUCXX)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/g++")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/msvc")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/clang")
	else ()
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/unknown")
	endif ()

	if (EVMJIT)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/JIT")
	else ()
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/int")
	endif ()

	if (PARANOID)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/PARA")
	endif ()

	#cmake build type may be not specified when using msvc
	if (CMAKE_BUILD_TYPE)
		set(_cmake_build_type ${CMAKE_BUILD_TYPE})
	else()
		set(_cmake_build_type "${CMAKE_CFG_INTDIR}")
	endif()

	message("createBuildInfo()")

	# Generate header file containing useful build information
	add_custom_target(BuildInfo.h ALL
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DETH_DST_DIR="${CMAKE_BINARY_DIR}" -DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DETH_BUILD_TYPE="${_cmake_build_type}" -DETH_BUILD_PLATFORM="${ETH_BUILD_PLATFORM}"
			-DPROJECT_VERSION="${PROJECT_VERSION}" -DETH_FATDB="${FATDB10}" -P "${ETH_SCRIPTS_DIR}/buildinfo.cmake"
		)

	# Generate header file containing configuration info
	if (FATDB)
		set(FATDB10 1)
	else()
		set(FATDB10 0)
	endif()
	add_custom_target(ConfigInfo.h ALL
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DETH_DST_DIR="${CMAKE_BINARY_DIR}" -DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DETH_FATDB="${FATDB10}" -DETH_HEADERFILE="ConfigInfo" -P "${ETH_SCRIPTS_DIR}/buildinfo.cmake"
		)

	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	set(SRC_LIST BuildInfo.h)
	set(SRC_LIST ConfigInfo.h)
endfunction()



######################################################################################################


# Clear invalid option
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	if (PARANOID)
		message("Paranoia requires debug - disabling for release build.")
		set(PARANOID OFF)
	endif ()
	if (VMTRACE)
		message("VM Tracing requires debug - disabling for release build.")
		set (VMTRACE OFF)
	endif ()
endif ()

# Force chromium.
set (ETH_HAVE_WEBENGINE 1)

# Backwards compatibility
if (HEADLESS)
	message("*** WARNING: -DHEADLESS=1 option is DEPRECATED! Use -DBUNDLE=minimal or -DGUI=0")
	set(GUI OFF)
endif ()

macro(eth_format_option O)
	if (${${O}})
		set(${O} ON)
	elseif ("${${O}}" STREQUAL "" AND ${D_${O}})
		set(${O} ON)
	else()
		set(${O} OFF)
	endif()
endmacro()

macro(jsonrcpstub_client_create SPEC CLIENTNAME CLIENTFILENAME)
	if (ETH_JSON_RPC_STUB)
		add_custom_target(${SPEC}stub)
		add_custom_command(
		TARGET ${SPEC}stub
		POST_BUILD
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SPEC}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SPEC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${SPEC}" -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DETH_CLIENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
			-DETH_CLIENT_NAME=${CLIENTNAME}
			-DETH_CLIENT_FILENAME=${CLIENTFILENAME}
			-DETH_JSON_RPC_STUB="${ETH_JSON_RPC_STUB}"
			-P "${ETH_SCRIPTS_DIR}/jsonrpcstub.cmake"
			)
		add_dependencies(${EXECUTABLE} ${SPEC}stub)
	endif ()
endmacro()

# Normalise build options
eth_format_option(PARANOID)
eth_format_option(VMTRACE)
eth_format_option(EVMJIT)
eth_format_option(FATDB)
eth_format_option(MINER)
eth_format_option(USENPM)
eth_format_option(PROFILING)
eth_format_option(ROCKSDB)
eth_format_option(GUI)
eth_format_option(TESTS)
eth_format_option(ROCKSDB)
eth_format_option(TOOLS)
eth_format_option(ETHKEY)
eth_format_option(ETHASHCL)
eth_format_option(JSCONSOLE)
eth_format_option(OLYMPIC)
eth_format_option(SERPENT)

# Default CMAKE_BUILD_TYPE accordingly.
set(CMAKE_BUILD_TYPE CACHE STRING ${D_CMAKE_BUILD_TYPE})

# Default TARGET_PLATFORM to ${CMAKE_SYSTEM_NAME}
# change this once we support cross compiling
set(TARGET_PLATFORM CACHE STRING ${CMAKE_SYSTEM_NAME})
if ("x${TARGET_PLATFORM}" STREQUAL "x")
	set(TARGET_PLATFORM ${CMAKE_SYSTEM_NAME})
endif ()

include(EthDependencies)

configureProject()

message("------------------------------------------------------------------------")
message("--                  CMake Version                            ${CMAKE_VERSION}")
message("-- CMAKE_BUILD_TYPE Build type                               ${CMAKE_BUILD_TYPE}")
message("-- TARGET_PLATFORM  Target platform                          ${TARGET_PLATFORM}")
message("-- BUNDLE           Build bundle                             ${BUNDLE}")
message("--------------------------------------------------------------- features")
message("--                  Chromium support                         ${ETH_HAVE_WEBENGINE}")
message("--                  Hardware identification support          ${CPUID_FOUND}")
message("--                  HTTP Request support                     ${CURL_FOUND}")
message("-- VMTRACE          VM execution tracing                     ${VMTRACE}")
message("-- PROFILING        Profiling support                        ${PROFILING}")
message("-- FATDB            Full database exploring                  ${FATDB}")
message("-- USENPM           Javascript source building               ${USENPM}")
message("-- ROCKSDB          Prefer rocksdb to leveldb                ${ROCKSDB}")
message("-- OLYMPIC          Default to the Olympic network           ${OLYMPIC}")
message("------------------------------------------------------------- components")
message("-- MINER            Build miner                              ${MINER}")
message("-- ETHKEY           Build wallet tools                       ${ETHKEY}")
message("-- TOOLS            Build basic tools                        ${TOOLS}")
message("-- SERPENT          Build Serpent language components        ${SERPENT}")
message("-- GUI              Build GUI components                     ${GUI}")
message("-- TESTS            Build tests                              ${TESTS}")
message("-- ETHASHCL         Build OpenCL components                  ${ETHASHCL}")
message("-- JSCONSOLE        Build with javascript console            ${JSCONSOLE}")
message("-- EVMJIT           Build LLVM-based JIT EVM                 ${EVMJIT}")
message("------------------------------------------------------------------------")
message("")

set(CMAKE_THREAD_LIBS_INIT pthread)

include(EthCompilerSettings)
message("-- CXXFLAGS: ${CMAKE_CXX_FLAGS}")

# this must be an include, as a function it would mess up with variable scope!
include(EthExecutableHelper)

message("creating build info...")
createBuildInfo()

if (ROCKSDB AND ROCKSDB_FOUND)
	set(DB_INCLUDE_DIRS ${ROCKSDB_INCLUDE_DIRS})
	set(DB_LIBRARIES ${ROCKSDB_LIBRARIES})
	add_definitions(-DETH_ROCKSDB)
else()
	set(DB_INCLUDE_DIRS ${LEVELDB_INCLUDE_DIRS})
	set(DB_LIBRARIES ${LEVELDB_LIBRARIES})
endif()

if (EVMJIT)
	if (NOT DEFINED LLVM_DIR)
		if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
			set(LLVM_DIR "${CMAKE_SOURCE_DIR}/extdep/install/windows/x64/share/llvm/cmake")
		elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			set(LLVM_DIR "/usr/local/opt/llvm/share/llvm/cmake")
		endif()
	endif()

	set(EVMJIT_CPP TRUE) # include CPP-JIT connector
	add_subdirectory(evmjit)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set(EVMJIT_DLLS_LOCAL $<TARGET_FILE:evmjit>)
		set(EVMJIT_DLLS optimized ${EVMJIT_DLLS_LOCAL} debug ${EVMJIT_DLLS_LOCAL})
	endif()
endif()

if (TOOLS OR GUI OR TESTS)
	set(GENERAL 1)
else ()
	set(GENERAL 0)
endif ()

add_subdirectory(libdevcore)
if (GENERAL)
	add_subdirectory(libevmcore)
	add_subdirectory(libevmasm)
	add_subdirectory(liblll)
endif ()

if (SERPENT)
	add_subdirectory(libserpent)
	add_subdirectory(sc)
endif ()

if (TOOLS)
	add_subdirectory(lllc)
endif ()

if (JSCONSOLE)
	add_subdirectory(libjsengine)
	add_subdirectory(libjsconsole)
endif ()

if (NOT WIN32)
	add_definitions(-DETH_HAVE_SECP256K1)
	add_subdirectory(secp256k1)
endif ()

add_subdirectory(libscrypt)
add_subdirectory(libdevcrypto)
add_subdirectory(libnatspec)

if (GENERAL)
	add_subdirectory(libp2p)
endif ()

if (GENERAL OR MINER)
	add_subdirectory(libethash)
	if (ETHASHCL)
		add_subdirectory(libethash-cl)
	endif ()
endif ()

add_subdirectory(libethcore)

if (GENERAL)
	add_subdirectory(libevm)
	add_subdirectory(libethereum)
endif ()

if (MINER OR TOOLS)
	add_subdirectory(ethminer)
endif ()

if (ETHKEY OR TOOLS)
	add_subdirectory(ethkey)
endif ()

# TODO: sort out tests so they're not dependent on webthree/web3jsonrpc and reenable
#if (TESTS)
	add_subdirectory(libtestutils)
	add_subdirectory(test)
#endif ()

if (TOOLS)

	add_subdirectory(rlp)
	add_subdirectory(abi)
	add_subdirectory(ethvm)

#	if("x${CMAKE_BUILD_TYPE}" STREQUAL "xDebug")
#		add_subdirectory(exp)
#	endif ()

endif()

# Optional sub-projects
if (EXISTS "${CMAKE_SOURCE_DIR}/solidity/CMakeLists.txt")
	add_subdirectory(solidity)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/webthree/CMakeLists.txt")
	add_subdirectory(webthree)
endif()

# Optionally if the user has dot/grapviz, generate the dependency graph
find_program(GRAPHVIZDOT dot)
if (GRAPHVIZDOT)
	message(STATUS "Graphviz detected. Drawing a module dependency graph at ${CMAKE_SOURCE_DIR}/dependency_graph.svg")
	execute_process(
		COMMAND dot -Tsvg dependency_graph.dot -o dependency_graph.svg
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
endif()

