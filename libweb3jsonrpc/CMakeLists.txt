aux_source_directory(. SRC_LIST)

set(EXECUTABLE web3jsonrpc)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::whisper Web3::webthree JsonRpc::Server)

if (ETH_JSON_RPC_STUB)
	add_custom_target(jsonrpcstub) 
	add_custom_command(TARGET jsonrpcstub
		POST_BUILD
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND}
		-DETH_SPEC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/spec.json"
		-DETH_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
		-DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DETH_SERVER_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DETH_CLIENT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
			-DETH_SERVER_NAME=AbstractWebThreeStubServer -DETH_CLIENT_NAME=WebThreeStubClient
			-DETH_JSON_RPC_STUB="${ETH_JSON_RPC_STUB}"
			-P "${ETH_SCRIPTS_DIR}/jsonrpcstub.cmake"
	)
	add_dependencies(${EXECUTABLE} jsonrpcstub)
endif ()

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

add_custom_target(aux_json SOURCES "spec.json")
