add_subdirectory(ethrpctest)
add_subdirectory(libweb3testutils)

include_directories(..)
include_directories(../..)
include_directories(BEFORE ${JSONCPP_INCLUDE_DIRS})

aux_source_directory(. SRCS)
aux_source_directory(./libwhisper SRCS)
file(GLOB HEADERS "*.h")
add_executable(testweb3 ${SRCS} ${HEADERS})

target_link_libraries(testweb3 webthree)
target_link_libraries(testweb3 web3jsonrpc)
target_link_libraries(testweb3 ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})

#eth_use(testweb3 REQUIRED EthCore)
