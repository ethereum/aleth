aux_source_directory(. SRC_LIST)

set(EXECUTABLE eth)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::web3jsonrpc JsonRpc::Client Web3::webthree Eth::ethcore Eth::p2p Eth::devcrypto Eth::ethereum Eth::ethashseal)
eth_use(${EXECUTABLE} OPTIONAL Eth::evmjit)

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
