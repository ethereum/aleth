cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

set(EXECUTABLE eth)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
add_definitions(-DETH_JSONRPC)

find_package (json_rpc_cpp 0.4 REQUIRED)
target_link_libraries(${EXECUTABLE} webthree)
target_link_libraries(${EXECUTABLE} web3jsonrpc)
target_link_libraries(${EXECUTABLE} ${JSON_RPC_CPP_CLIENT_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${JSON_RPC_CPP_SERVER_LIBRARIES})


# eth_use(${EXECUTABLE} REQUIRED EthCore WebThreeJsonRpc)
eth_use(${EXECUTABLE} REQUIRED EthCore)

if (DEFINED WIN32 AND NOT DEFINED CMAKE_COMPILER_IS_MINGW)
	eth_copy_dlls("${EXECUTABLE}" CURL_DLLS)
endif()
if (READLINE_FOUND)
	target_link_libraries(${EXECUTABLE} ${READLINE_LIBRARIES})
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	eth_copy_dlls("${EXECUTABLE}" MHD_DLLS EVMJIT_DLLS OpenCL_DLLS)
endif()

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
