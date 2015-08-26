cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

include_directories(..)

set(EXECUTABLE ethrpctest)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED EthCore ServerJsonRpc)
target_link_libraries(${EXECUTABLE} webthree)
target_link_libraries(${EXECUTABLE} web3jsonrpc)

target_link_libraries(${EXECUTABLE} ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(${EXECUTABLE} ${ETH_TESTUTILS_LIBRARY})
target_link_libraries(${EXECUTABLE} ${JSON_RPC_CPP_CLIENT_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${JSON_RPC_CPP_SERVER_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${OpenCL_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${MINIUPNPC_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${CURL_LIBRARIES})
target_link_libraries(${EXECUTABLE} web3testutils)

install( TARGETS ${EXECUTABLE} DESTINATION bin )

