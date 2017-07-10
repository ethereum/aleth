aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_subdirectory(genesis)
add_subdirectory(genesis/test)
aux_source_directory(./genesis SRCS)
aux_source_directory(./genesis/test SRCS)
set(SRC_LIST ${SRC_LIST} ${SRCS})

add_library(ethashseal ${SRC_LIST} ${HEADERS})
eth_use(ethashseal OPTIONAL Eth::ethash-cl Cpuid)

find_package(Eth)

target_include_directories(ethashseal PRIVATE ..)
target_link_libraries(ethashseal ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethashseal ${Eth_ETHEREUM_LIBRARIES})
