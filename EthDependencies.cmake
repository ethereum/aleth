# all dependencies that are not directly included in the cpp-ethereum distribution are defined here
# for this to work, download the dependency via the cmake script in extdep or install them manually!

if (DEFINED MSVC)
	# by defining CMAKE_PREFIX_PATH variable, cmake will look for dependencies first in our own repository before looking in system paths like /usr/local/ ...
	# this must be set to point to the same directory as $ETH_DEPENDENCY_INSTALL_DIR in /extdep directory
	set (ETH_DEPENDENCY_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../cpp-ethereum/extdep/install/windows/x64")
	set (CMAKE_PREFIX_PATH ${ETH_DEPENDENCY_INSTALL_DIR} ${CMAKE_PREFIX_PATH})

	# Qt5 requires opengl
	# TODO it windows SDK is NOT FOUND, throw ERROR
	# from https://github.com/rpavlik/cmake-modules/blob/master/FindWindowsSDK.cmake
	find_package(WINDOWSSDK REQUIRED)
	message(" - WindowsSDK dirs: ${WINDOWSSDK_DIRS}")
	set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${WINDOWSSDK_DIRS})

elseif (DEFINED APPLE)
	# homebrew install directories for few of our dependencies
	set (CMAKE_PREFIX_PATH "/usr/local/opt/qt5" ${CMAKE_PREFIX_PATH})
	set (CMAKE_PREFIX_PATH "/usr/local/opt/v8-315" ${CMAKE_PREFIX_PATH})
	set (CMAKE_PREFIX_PATH "/usr/local/Cellar/readline/6.3.8" ${CMAKE_PREFIX_PATH})
endif()

message(STATUS "CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")

find_package(Eth)

# setup directory for cmake generated files and include it globally
# it's not used yet, but if we have more generated files, consider moving them to ETH_GENERATED_DIR
set(ETH_GENERATED_DIR "${PROJECT_BINARY_DIR}/gen")
include_directories(${ETH_GENERATED_DIR})

# boilerplate macros for some code editors
add_definitions(-DETH_TRUE)

# custom cmake scripts
set(ETH_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ETH_SCRIPTS_DIR ${ETH_CMAKE_DIR}/scripts)
message("CMake Helper Path: ${ETH_CMAKE_DIR}")
message("CMake Script Path: ${ETH_SCRIPTS_DIR}")

find_program(CTEST_COMMAND ctest)
message(STATUS "ctest path: ${CTEST_COMMAND}")

# Dependencies must have a version number, to ensure reproducible build. The version provided here is the one that is in the extdep repository. If you use system libraries, version numbers may be different.

find_package (CryptoPP 5.6.2 EXACT REQUIRED)
message(STATUS "CryptoPP header: ${CRYPTOPP_INCLUDE_DIRS}")
message(STATUS "CryptoPP lib   : ${CRYPTOPP_LIBRARIES}")

find_package (LevelDB REQUIRED)
message(STATUS "LevelDB header: ${LEVELDB_INCLUDE_DIRS}")
message(STATUS "LevelDB lib: ${LEVELDB_LIBRARIES}")

find_package (RocksDB)
message(STATUS "RocksDB header: ${ROCKSDB_INCLUDE_DIRS}")
message(STATUS "RocksDB lib: ${ROCKSDB_LIBRARIES}")

if (ROCKSDB AND ROCKSDB_FOUND)
	set(DB_INCLUDE_DIRS ${ROCKSDB_INCLUDE_DIRS})
	set(DB_LIBRARIES ${ROCKSDB_LIBRARIES})
	add_definitions(-DETH_ROCKSDB)
else()
	set(DB_INCLUDE_DIRS ${LEVELDB_INCLUDE_DIRS})
	set(DB_LIBRARIES ${LEVELDB_LIBRARIES})
endif()

find_package (v8)
message(STATUS "v8 header: ${V8_INCLUDE_DIRS}")
message(STATUS "v8 lib   : ${V8_LIBRARIES}")

if (JSCONSOLE)
	add_definitions(-DETH_JSCONSOLE)
endif()

# TODO the Jsoncpp package does not yet check for correct version number
find_package (Jsoncpp 0.60 REQUIRED)
message(STATUS "Jsoncpp header: ${JSONCPP_INCLUDE_DIRS}")
message(STATUS "Jsoncpp lib   : ${JSONCPP_LIBRARIES}")

# TODO get rid of -DETH_JSONRPC
find_package (json_rpc_cpp 0.4)
message(STATUS "json-rpc-cpp header: ${JSON_RPC_CPP_INCLUDE_DIRS}")
message(STATUS "json-rpc-cpp lib   : ${JSON_RPC_CPP_LIBRARIES}")

find_package(MHD)
message(STATUS "microhttpd header: ${MHD_INCLUDE_DIRS}")
message(STATUS "microhttpd lib   : ${MHD_LIBRARIES}")
message(STATUS "microhttpd dll   : ${MHD_DLLS}")

# TODO readline package does not yet check for correct version number
# TODO make readline package dependent on cmake options
# TODO get rid of -DETH_READLINE
find_package (Readline 6.3.8)
message(STATUS "readline header: ${READLINE_INCLUDE_DIRS}")
message(STATUS "readline lib   : ${READLINE_LIBRARIES}")

# TODO miniupnpc package does not yet check for correct version number
# TODO make miniupnpc package dependent on cmake options
# TODO get rid of -DMINIUPNPC
find_package (Miniupnpc 1.8.2013)
message(STATUS "miniupnpc header: ${MINIUPNPC_INCLUDE_DIRS}")
message(STATUS "miniupnpc lib   : ${MINIUPNPC_LIBRARIES}")

# TODO gmp package does not yet check for correct version number
# TODO it is also not required in msvc build
find_package (Gmp 6.0.0)
message(" - gmp header: ${GMP_INCLUDE_DIRS}")
message(" - gmp lib   : ${GMP_LIBRARIES}")

# curl is only requried for tests
# TODO specify min curl version, on windows we are currently using 7.29
find_package (CURL)
message(STATUS "curl header: ${CURL_INCLUDE_DIRS}")
message(STATUS "curl lib   : ${CURL_LIBRARIES}")

# cpuid required for eth
find_package (Cpuid)
message(STATUS "cpuid header: ${CPUID_INCLUDE_DIRS}")
message(STATUS "cpuid lib   : ${CPUID_LIBRARIES}")

find_package (OpenCL)
message(STATUS "opencl header: ${OpenCL_INCLUDE_DIRS}")
message(STATUS "opencl lib   : ${OpenCL_LIBRARIES}")

# find location of jsonrpcstub
find_program(ETH_JSON_RPC_STUB jsonrpcstub)
message(STATUS "jsonrpcstub location    : ${ETH_JSON_RPC_STUB}")

# TODO: review this and decide if it's necessary
# do not compile GUI
#if (GUI)

## find all of the Qt packages
## remember to use 'Qt' instead of 'QT', cause unix is case sensitive
## TODO make headless client optional

	#set (ETH_QT_VERSION 5.4)

	#if (USENPM)

		## TODO check node && npm version
		#find_program(ETH_NODE node)
		#string(REGEX REPLACE "node" "" ETH_NODE_DIRECTORY ${ETH_NODE})
		#message(" - nodejs location : ${ETH_NODE}")

		#find_program(ETH_NPM npm)
		#string(REGEX REPLACE "npm" "" ETH_NPM_DIRECTORY ${ETH_NPM})
		#message(" - npm location    : ${ETH_NPM}")

		#if (NOT ETH_NODE)
			#message(FATAL_ERROR "node not found!")
		#endif()
		#if (NOT ETH_NPM)
			#message(FATAL_ERROR "npm not found!")
		#endif()
	#endif()

#endif() #GUI

## use multithreaded boost libraries, with -mt suffix
set(Boost_USE_MULTITHREADED ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

# TODO hanlde other msvc versions or it will fail find them
	set(Boost_COMPILER -vc120)
# use static boost libraries *.lib
	set(Boost_USE_STATIC_LIBS ON)

elseif (APPLE)

# use static boost libraries *.a
	set(Boost_USE_STATIC_LIBS ON)

elseif (UNIX)
# use dynamic boost libraries *.dll
	set(Boost_USE_STATIC_LIBS OFF)

endif()

find_package(Boost 1.54.0 REQUIRED COMPONENTS thread date_time system regex chrono filesystem unit_test_framework program_options random)

message(" - boost header: ${Boost_INCLUDE_DIRS}")
message(" - boost lib   : ${Boost_LIBRARIES}")

if (APPLE)
	link_directories(/usr/local/lib)
	include_directories(/usr/local/include)
endif()

function(eth_use TARGET REQUIRED)
	foreach(MODULE ${ARGN})
		string(REPLACE "::" ";" MODULE_PARTS ${MODULE})
		list(GET MODULE_PARTS 0 MODULE_MAIN)
		list(LENGTH MODULE_PARTS MODULE_LENGTH)
		if (MODULE_LENGTH GREATER 1)
			list(GET MODULE_PARTS 1 MODULE_SUB)
		endif()
		# TODO: check if file exists if not, throws FATAL_ERROR with detailed description
		include(Use${MODULE_MAIN})
		eth_apply(${TARGET} ${REQUIRED} ${MODULE_SUB})
	endforeach()
endfunction()
