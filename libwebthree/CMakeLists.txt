file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(webthree ${SRC_LIST} ${HEADERS})

eth_use(webthree REQUIRED Web3::whisper)

find_package(Eth)
find_package(Web3)

target_include_directories(webthree PRIVATE ..)
target_link_libraries(webthree ${Eth_ETHASH_LIBRARIES})
target_link_libraries(webthree ${Eth_ETHASHSEAL_LIBRARIES})
target_link_libraries(webthree ${Web3_WHISPER_LIBRARIES})

install(TARGETS webthree RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
