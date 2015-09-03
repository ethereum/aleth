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

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE ${D_CMAKE_BUILD_TYPE})
endif ()



		#add_definitions(-DETH_ETHASHCL)
	#endif()

	#if (EVMJIT)
		#add_definitions(-DETH_EVMJIT)
	#endif()

	#if (CPUID_FOUND)
		#add_definitions(-DETH_CPUID)
	#endif()

	#if (CURL_FOUND)
		#add_definitions(-DETH_CURL)
	#endif()



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
	if (FATDB)
		set(FATDB10 1)
	else()
		set(FATDB10 0)
	endif()
	add_custom_target(BuildInfo.h ALL
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DETH_DST_DIR="${CMAKE_BINARY_DIR}"
			-DETH_BUILD_TYPE="${_cmake_build_type}" -DETH_BUILD_PLATFORM="${ETH_BUILD_PLATFORM}" -DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DPROJECT_VERSION="${PROJECT_VERSION}" -DETH_FATDB="${FATDB10}"
			-P "${ETH_SCRIPTS_DIR}/buildinfo.cmake"
		)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	set(SRC_LIST BuildInfo.h)
endfunction()

######################################################################################################

# Force chromium.
set (ETH_HAVE_WEBENGINE 1)

# Backwards compatibility
if (HEADLESS)
	message("*** WARNING: -DHEADLESS=1 option is DEPRECATED! Use -DBUNDLE=minimal or -DGUI=0")
	set(GUI OFF)
endif ()




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

