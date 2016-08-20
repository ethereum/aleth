
aux_source_directory(. SRCS)
aux_source_directory(./libwhisper SRCS)
aux_source_directory(./libweb3jsonrpc SRCS)
enable_bulkbuild(testweb3 SRCS)

file(GLOB HEADERS "*.h")
add_executable(testweb3 ${SRCS} ${HEADERS})

eth_use(testweb3 REQUIRED JsonRpc::Client)

find_package(Boost QUIET REQUIRED COMPONENTS unit_test_framework)
find_package(Web3)

target_include_directories(testweb3 PRIVATE ..)
target_include_directories(testweb3 PRIVATE ../../..)
target_include_directories(testweb3 PRIVATE ../../../utils)

# Messy.  The webthree tests depend on the libethereum tests (TestHelper.h)
target_include_directories(testweb3 PRIVATE ../../libethereum)

target_link_libraries(testweb3 ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(testweb3 ${Web3_WEBTHREE_LIBRARIES})
target_link_libraries(testweb3 ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(testweb3 ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(testweb3 ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(testweb3 pthread)
endif()
