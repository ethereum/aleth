add_subdirectory(ethrpctest)

aux_source_directory(. SRCS)
aux_source_directory(./libwhisper SRCS)
file(GLOB HEADERS "*.h")
add_executable(testweb3 ${SRCS} ${HEADERS})

eth_use(testweb3 REQUIRED Web3::web3jsonrpc JsonRpc::Client)

target_link_libraries(testweb3 ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
