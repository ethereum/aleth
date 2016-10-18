aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(eth ${SRC_LIST} ${HEADERS})

find_package(Eth)
find_package(Web3)

target_include_directories(eth PRIVATE ..)
target_link_libraries(eth ${Eth_EVM_LIBRARIES})
target_link_libraries(eth ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(eth ${Web3_WEBTHREE_LIBRARIES})
target_link_libraries(eth jsonrpc::client)
# TODO: Remove.
target_compile_definitions(eth PRIVATE ETH_JSONRPC)

if (EVMJIT)
	# Do we need include paths here?
	target_link_libraries(eth ${Eth_EVMJIT_LIBRARIES})
	eth_copy_dlls(eth EVMJIT_DLLS)
endif()

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(eth ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(eth ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(eth pthread)
endif()

if (APPLE)
	install(TARGETS eth DESTINATION bin)
else()
	eth_install_executable(eth)
endif()
