cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

include_directories(BEFORE ..)
include_directories(${Boost_INCLUDE_DIRS})
if (JSONRPC)
include_directories(BEFORE ${JSONCPP_INCLUDE_DIRS})
include_directories(${JSON_RPC_CPP_INCLUDE_DIRS})
endif()

set(EXECUTABLE ethminer)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

add_dependencies(${EXECUTABLE} BuildInfo.h)

target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})

if (JSONRPC)
	target_link_libraries(${EXECUTABLE} ${JSON_RPC_CPP_CLIENT_LIBRARIES})
	target_link_libraries(${EXECUTABLE} ${CURL_LIBRARIES})
	if (DEFINED WIN32 AND NOT DEFINED CMAKE_COMPILER_IS_MINGW)
		eth_copy_dlls(${EXECUTABLE} CURL_DLLS)
	endif()
endif()

if (ETH_JSON_RPC_STUB)
	add_custom_command(
		POST_BUILD
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/farm.json"
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/FarmClient.h"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SPEC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/farm.json" -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
			-DETH_CLIENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
			-DETH_CLIENT_NAME=FarmClient
			-DETH_JSON_RPC_STUB="${ETH_JSON_RPC_STUB}"
			-P "${ETH_SCRIPTS_DIR}/jsonrpcstub.cmake"
	)

	add_custom_command(
		POST_BUILD
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/phonehome.json"
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/PhoneHome.h"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SPEC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/phonehome.json" -DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
			-DETH_CLIENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
			-DETH_CLIENT_NAME=PhoneHome
			-DETH_JSON_RPC_STUB="${ETH_JSON_RPC_STUB}"
			-P "${ETH_SCRIPTS_DIR}/jsonrpcstub.cmake"
	)
endif ()

target_link_libraries(${EXECUTABLE} ethcore)
target_link_libraries(${EXECUTABLE} ethash)
target_link_libraries(${EXECUTABLE} devcrypto)

if (DEFINED WIN32 AND NOT DEFINED CMAKE_COMPILER_IS_MINGW)
	eth_copy_dlls("${EXECUTABLE}" MHD_DLLS)
	eth_copy_dlls("${EXECUTABLE}" OpenCL_DLLS)
endif()

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
