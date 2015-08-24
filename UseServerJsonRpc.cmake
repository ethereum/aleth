function(eth_apply TARGET REQUIRED)	
	find_package(json_rpc_cpp 0.4 REQUIRED)
	message (" - json-rpc-cpp header: ${JSON_RPC_CPP_INCLUDE_DIRS}")
	message (" - json-rpc-cpp lib   : ${JSON_RPC_CPP_LIBRARIES}")
	add_definitions(-DETH_JSONRPC)

	find_package(MHD)
	message(" - microhttpd header: ${MHD_INCLUDE_DIRS}")
	message(" - microhttpd lib   : ${MHD_LIBRARIES}")
	message(" - microhttpd dll   : ${MHD_DLLS}")

	include_directories(BEFORE SYSTEM ${JSONCPP_INCLUDE_DIR})	
	include_directories(SYSTEM ${JSON_RPC_CPP_INCLUDE_DIRS})
	target_link_libraries(${TARGET} ${JSON_RPC_CPP_SERVER_LIBRARIES})
	target_link_libraries(${TARGET} ${JSONCPP_LIBRARIES})
	target_link_libraries(${TARGET} ${MHD_LIBRARIES})

	eth_copy_dlls(${TARGET} MHD_DLLS)

	# find location of jsonrpcstub
	find_program(ETH_JSON_RPC_STUB jsonrpcstub)
	message(" - jsonrpcstub location    : ${ETH_JSON_RPC_STUB}")

endfunction()
