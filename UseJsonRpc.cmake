function(eth_apply TARGET REQUIRED SUBMODULE)	

	eth_use(${TARGET} ${REQUIRED} Jsoncpp)

	if (${SUBMODULE} STREQUAL "Server")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${JSON_RPC_CPP_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${JSON_RPC_CPP_SERVER_LIBRARIES})
		target_link_libraries(${TARGET} ${MHD_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_JSONRPC)

		eth_copy_dlls(${TARGET} MHD_DLLS)
	endif()

	if (${SUBMODULE} STREQUAL "Client")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${JSON_RPC_CPP_INCLUDE_DIRS})
		target_include_directories(${TARGET} SYSTEM PUBLIC ${CURL_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${JSON_RPC_CPP_CLIENT_LIBRARIES})
		target_link_libraries(${TARGET} ${CURL_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_JSONRPC)

		eth_copy_dlls(${TARGET} CURL_DLLS)
	endif()

endfunction()
