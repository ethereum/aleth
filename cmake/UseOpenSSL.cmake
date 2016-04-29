function(eth_apply TARGET REQUIRED)
	find_package (OpenSSL)

	# cmake supplied FindOpenSSL doesn't set all our variables
	set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})

	eth_show_dependency(OPENSSL OpenSSL)
	if (OPENSSL_FOUND)
		target_include_directories(${TARGET} SYSTEM PUBLIC ${OPENSSL_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${OPENSSL_LIBRARIES})
	elseif (NOT ${REQUIRED} STREQUAL "OPTIONAL")
		message(FATAL_ERROR "OpenSSL library not found")
	endif()
endfunction()
