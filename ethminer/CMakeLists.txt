aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(ethminer ${SRC_LIST} ${HEADERS})

jsonrpcstub_client_create(ethminer minerfarm.json FarmClient ${CMAKE_CURRENT_SOURCE_DIR} FarmClient)

find_package(Eth)

target_include_directories(ethminer PRIVATE ..)
target_link_libraries(ethminer ${Eth_ETHASHSEAL_LIBRARIES} jsonrpc::client)
# TODO: Remove.
target_compile_definitions(ethminer PRIVATE ETH_JSONRPC)

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(ethminer ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethminer ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(ethminer pthread)
endif()

if (APPLE)
	install(TARGETS ethminer DESTINATION bin)
else()
	eth_install_executable(ethminer)
endif()
