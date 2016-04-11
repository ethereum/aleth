aux_source_directory(. SRC_LIST)

include_directories(..)
include_directories(../..)

set(EXECUTABLE ethrpctest)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::web3jsonrpc Eth::testutils JsonRpc::Client)
target_link_libraries(${EXECUTABLE} ${Boost_PROGRAM_OPTIONS_LIBRARY})

install( TARGETS ${EXECUTABLE} DESTINATION bin )

