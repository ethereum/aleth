set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethereum)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
eth_use(${EXECUTABLE} REQUIRED Eth::evm Eth::ethcore)
if (NOT EMSCRIPTEN)
	eth_use(${EXECUTABLE} REQUIRED Dev::p2p Dev::devcrypto)
endif()
target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )

