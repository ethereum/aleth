aux_source_directory(. SRC_LIST)

set(EXECUTABLE bench)

add_executable(${EXECUTABLE} ${SRC_LIST})

eth_use(${EXECUTABLE} REQUIRED Dev::devcrypto)

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()

