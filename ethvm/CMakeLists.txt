aux_source_directory(. SRC_LIST)

add_executable(ethvm ${SRC_LIST})

find_package(Eth)

target_include_directories(ethvm PRIVATE ..)
target_link_libraries(ethvm ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethvm ${Eth_ETHASHSEAL_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(ethvm pthread)
endif()

install(TARGETS ethvm DESTINATION bin OPTIONAL)
