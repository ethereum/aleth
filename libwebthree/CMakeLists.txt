add_library(webthree WebThree.cpp WebThree.h)

eth_use(webthree REQUIRED Web3::whisper)

find_package(Eth)
find_package(Web3)

target_include_directories(webthree PRIVATE ..)
target_link_libraries(webthree ${Eth_ETHASH_LIBRARIES})
target_link_libraries(webthree ${Eth_ETHASHSEAL_LIBRARIES})
target_link_libraries(webthree ${Web3_WHISPER_LIBRARIES})
