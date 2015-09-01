function(eth_apply TARGET REQUIRED SUBMODULE)

	find_package(Utils)

	target_include_directories(${TARGET} SYSTEM BEFORE PUBLIC ${Utils_INCLUDE_DIRS})

	if (${SUBMODULE} STREQUAL "secp256k1")
		eth_use(${EXECUTABLE} ${REQUIRED} Gmp)
		target_link_libraries(${TARGET} ${Utils_SECP256K1_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_HAVE_SECP256K1)
	endif()

	if (${SUBMODULE} STREQUAL "scrypt")
		target_link_libraries(${TARGET} ${Utils_SCRYPT_LIBRARIES})
	endif()

endfunction()
