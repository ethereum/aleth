aux_source_directory(. SRC_LIST)

add_executable(ethvm ${SRC_LIST})

find_package(Eth)

target_include_directories(ethvm PRIVATE ..)
target_link_libraries(ethvm ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethvm ${Eth_ETHASHSEAL_LIBRARIES})

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(ethvm ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethvm ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(ethvm pthread)
endif()

eth_install_executable(ethvm)
