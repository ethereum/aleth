set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

include_directories(..)

set(EXECUTABLE web3testutils)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED EthCore ServerJsonRpc)
target_link_libraries(${EXECUTABLE} webthree)
target_link_libraries(${EXECUTABLE} web3jsonrpc)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )
