file(GLOB sources "*.cpp" "*.h")

add_library(p2p ${sources})
target_link_libraries(p2p PUBLIC devcrypto devcore)
target_include_directories(p2p SYSTEM PRIVATE ${CRYPTOPP_INCLUDE_DIR})
eth_use(p2p OPTIONAL Miniupnpc)
