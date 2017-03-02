aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_executable(ethminer ${SRC_LIST} ${HEADERS})

jsonrpcstub_client_create(ethminer minerfarm.json FarmClient ${CMAKE_CURRENT_SOURCE_DIR} FarmClient)

find_package(Eth)

target_include_directories(ethminer PRIVATE ..)
target_link_libraries(ethminer ${Eth_ETHASHSEAL_LIBRARIES} JsonRpcCpp::Client)

if (UNIX AND NOT APPLE)
	target_link_libraries(ethminer pthread)
endif()

install(TARGETS ethminer DESTINATION bin OPTIONAL)
