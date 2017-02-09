set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(p2p ${SRC_LIST} ${HEADERS})

find_package(Dev)

target_link_libraries(p2p devcrypto devcore)
target_include_directories(p2p PRIVATE ..)
target_include_directories(p2p SYSTEM PRIVATE ${CRYPTOPP_INCLUDE_DIR})
eth_use(p2p OPTIONAL Miniupnpc)

install(TARGETS p2p RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
