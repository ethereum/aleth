add_subdirectory(ethrpctest)

aux_source_directory(. SRCS)
aux_source_directory(./libwhisper SRCS)
aux_source_directory(./libweb3jsonrpc SRCS)

set(EXECUTABLE testweb3)

file(GLOB HEADERS "*.h")
add_executable(${EXECUTABLE} ${SRCS} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::web3jsonrpc JsonRpc::Client)

target_link_libraries(${EXECUTABLE} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
