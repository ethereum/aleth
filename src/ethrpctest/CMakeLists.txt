cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

include_directories(..)

set(EXECUTABLE ethrpctest)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED WebThreeJsonRpc)

target_link_libraries(${EXECUTABLE} ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(${EXECUTABLE} ${ETH_TESTUTILS_LIBRARY})
target_link_libraries(${EXECUTABLE} web3testutils)

install( TARGETS ${EXECUTABLE} DESTINATION bin )

