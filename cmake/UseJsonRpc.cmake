function(eth_apply TARGET REQUIRED SUBMODULE)	

	eth_use(${TARGET} ${REQUIRED} Jsoncpp)
	find_package (json_rpc_cpp 0.4)
	find_program(ETH_JSON_RPC_STUB jsonrpcstub)
	eth_show_dependency(JSON_RPC_CPP json-rpc-cpp)

	if (${SUBMODULE} STREQUAL "Server")
		eth_use(${TARGET} ${REQUIRED} Mhd)
		get_property(DISPLAYED GLOBAL PROPERTY ETH_JSONRPCSTUB_DISPLAYED)
		if (NOT DISPLAYED)
			set_property(GLOBAL PROPERTY ETH_JSONRPCSTUB_DISPLAYED TRUE)
			message(STATUS "jsonrpcstub location    : ${ETH_JSON_RPC_STUB}")
		endif()
		target_include_directories(${TARGET} SYSTEM PUBLIC ${JSON_RPC_CPP_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${JSON_RPC_CPP_SERVER_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_JSONRPC)
	endif()

	if (${SUBMODULE} STREQUAL "Client")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${JSON_RPC_CPP_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${JSON_RPC_CPP_CLIENT_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_JSONRPC)

		eth_use(${TARGET} ${REQUIRED} CURL)
	endif()

endfunction()
