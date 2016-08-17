set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

add_library(p2p ${SRC_LIST} ${HEADERS})

eth_use(p2p REQUIRED Cryptopp Miniupnpc)

find_package(Dev)

target_link_libraries(p2p ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(p2p ${Dev_DEVCRYPTO_LIBRARIES})

install(TARGETS p2p RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
