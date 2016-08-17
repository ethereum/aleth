set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")
list(APPEND SRC_LIST libexecstream/exec-stream.cpp)

add_library(webthree ${SRC_LIST} ${HEADERS})

eth_use(webthree REQUIRED Web3::whisper)

find_package(Eth)
find_package(Web3)

target_link_libraries(webthree ${Eth_ETHASH_LIBRARIES})
target_link_libraries(webthree ${Eth_ETHASHSEAL_LIBRARIES})
target_link_libraries(webthree ${Web3_WHISPER_LIBRARIES})

install(TARGETS webthree RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
