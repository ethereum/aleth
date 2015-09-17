function(eth_apply TARGET REQUIRED)
	# homebrew install directories for few of our dependencies
	set (CMAKE_PREFIX_PATH "/usr/local/opt/v8-315" ${CMAKE_PREFIX_PATH})
	find_package (v8 QUIET)
	eth_show_dependency(V8 c8)

	if (NOT V8_FOUND)
		if (NOT ${REQUIRED} STREQUAL "OPTIONAL")
			message(FATAL_ERROR "v8 library not found")
		endif()
		return()
	endif()

	# don't know why, but v8 don't want to compile with SYSTEM flag
	target_include_directories(${TARGET} BEFORE PUBLIC ${V8_INCLUDE_DIRS})
	target_link_libraries(${TARGET} ${V8_LIBRARIES})
endfunction()
