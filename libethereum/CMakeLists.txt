set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethereum)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
eth_use(${EXECUTABLE} REQUIRED Eth::evm Eth::ethcore)
if (NOT EMSCRIPTEN)
	eth_use(${EXECUTABLE} REQUIRED Eth::lll Dev::p2p Dev::devcrypto JsonRpc::Server JsonRpc::Client)
endif()
target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})

jsonrpcstub_client_create(Sentinel.json dev::eth::Sentinel ${CMAKE_CURRENT_SOURCE_DIR} Sentinel)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

