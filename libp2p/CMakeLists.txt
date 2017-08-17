file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(p2p ${SOURCES} ${HEADERS})

target_link_libraries(p2p devcrypto devcore)
target_include_directories(p2p PRIVATE ..)
target_include_directories(p2p SYSTEM PRIVATE ${CRYPTOPP_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
eth_use(p2p OPTIONAL Miniupnpc)
