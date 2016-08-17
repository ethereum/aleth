aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(eth ${SRC_LIST} ${HEADERS})

eth_use(eth REQUIRED JsonRpc::Client)

find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
find_package(Eth)
find_package(Web3)

if (DEFINED MSVC)
	target_link_libraries(eth ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(eth ${Boost_DATE_TIME_LIBRARIES})
endif()
target_link_libraries(eth ${Eth_EVM_LIBRARIES})
target_link_libraries(eth ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(eth ${Web3_WEBTHREE_LIBRARIES})
if (EVMJIT)
	target_link_libraries(eth ${Eth_EVMJIT_LIBRARIES})
endif()
if (UNIX AND NOT APPLE)
	target_link_libraries(eth pthread)
endif()

if (EVMJIT)
	eth_copy_dlls(eth EVMJIT_DLLS)
endif()

if (APPLE)
	install(TARGETS eth DESTINATION bin)
else()
	eth_install_executable(eth)
endif()
