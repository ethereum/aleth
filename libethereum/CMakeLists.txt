set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethereum)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
eth_use(${EXECUTABLE} REQUIRED Eth::evm Eth::lll Dev::p2p Dev::devcrypto Eth::ethcore JsonRpc::Server JsonRpc::Client)
target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})

jsonrcpstub_client_create(Sentinel.json dev::eth::Sentinel Sentinel)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

