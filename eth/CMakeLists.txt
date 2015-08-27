cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

set(EXECUTABLE eth)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::web3jsonrpc JsonRpc::Client Eth::jsconsole)
eth_use(${EXECUTABLE} OPTIONAL Readline)

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
