function(eth_apply TARGET REQUIRED SUBMODULE)

	if (DEFINED webthree_SOURCE_DIR)
		set(W3_DIR "${ethereum_SOURCE_DIR}/webthree")
	else()
		set(W3_DIR         "${PROJECT_SOURCE_DIR}/../webthree" 		CACHE PATH "The path to the webthree directory")
		set(W3_BUILD_DIR_NAME  "build"                            	CACHE STRING "The name of the build directory in web3")
		set(W3_BUILD_DIR   "${W3_DIR}/${W3_BUILD_DIR_NAME}")
		set(CMAKE_LIBRARY_PATH ${W3_BUILD_DIR};${CMAKE_LIBRARY_PATH})
	endif()

	find_package(Web3)
	target_include_directories(${TARGET} PUBLIC ${W3_DIR})

	if (${SUBMODULE} STREQUAL "whisper")
		eth_use(${TARGET} ${REQUIRED} Eth::devcore Eth::p2p Eth::devcrypto)
		target_link_libraries(${TARGET} ${Web3_WHISPER_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "webthree")
		eth_use(${TARGET} ${REQUIRED} Web3::whisper Eth::devcore Eth::p2p Eth::devcrypto Eth::ethereum)
		target_link_libraries(${TARGET} ${Web3_WEBTHREE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "web3jsonrpc")
		eth_use(${TARGET} ${REQUIRED} Web3::webthree JsonRpc::Server)
		target_link_libraries(${TARGET} ${Web3_WEB3JSONRPC_LIBRARIES})
	endif()

endfunction()
