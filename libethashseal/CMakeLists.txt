aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

set(SRC_LIST ${SRC_LIST} ${SRCS})

add_library(ethashseal ${SRC_LIST} ${HEADERS})

find_package(Eth)

target_include_directories(ethashseal PRIVATE ..)
target_link_libraries(ethashseal ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethashseal ${Eth_ETHEREUM_LIBRARIES})
