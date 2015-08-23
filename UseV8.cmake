function(eth_apply TARGET REQUIRED)
	find_package (v8 REQUIRED)
	message(" - v8 header: ${V8_INCLUDE_DIRS}")
	message(" - v8 lib   : ${V8_LIBRARIES}")

	include_directories(BEFORE SYSTEM ${V8_INCLUDE_DIRS})
	target_link_libraries(${TARGET} ${V8_LIBRARIES})
endfunction()
