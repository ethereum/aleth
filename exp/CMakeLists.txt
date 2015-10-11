aux_source_directory(. SRC_LIST)

set(EXECUTABLE exp)

add_executable(${EXECUTABLE} ${SRC_LIST})

eth_use(${EXECUTABLE} REQUIRED Dev::devcore)
eth_use(${EXECUTABLE} REQUIRED Web3::webthree)
target_link_libraries(${EXECUTABLE} ${Boost_IOSTREAMS_LIBRARIES})

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()

