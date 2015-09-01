function(eth_apply TARGET REQUIRED )

	set(SOL_DIR "${ETH_CMAKE_DIR}/../solidity" CACHE PATH "The path to the solidity directory")
	set(SOL_BUILD_DIR_NAME  "build" CACHE STRING "The name of the build directory in solidity repo")
	set(SOL_BUILD_DIR "${SOL_DIR}/${SOL_BUILD_DIR_NAME}")
	set(CMAKE_LIBRARY_PATH ${SOL_BUILD_DIR};${CMAKE_LIBRARY_PATH})

	find_package(Solidity)

	target_include_directories(${TARGET} PUBLIC ${SOLIDITY_INCLUDE_DIRS})

	message("Using Solidity library: ${SOLIDITY_LIBRARIES}")

	eth_use(${TARGET} Eth::devcore Eth::evmasm)
	target_compile_definitions(${TARGET} PUBLIC ETH_SOLIDITY)
	target_link_libraries(${TARGET} ${SOLIDITY_LIBRARIES})
endfunction()
