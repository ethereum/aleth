aux_source_directory(. SRC_LIST)
include_directories(..)

add_executable(ethvm ${SRC_LIST})

find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
find_package(Eth)

if (DEFINED MSVC)
	target_link_libraries(ethvm ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethvm ${Boost_DATE_TIME_LIBRARIES})
endif()
target_link_libraries(ethvm ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethvm ${Eth_ETHASHSEAL_LIBRARIES})
if (UNIX AND NOT APPLE)
	target_link_libraries(ethvm pthread)
endif()

if (APPLE)
	install(TARGETS ethvm DESTINATION bin)
else()
	eth_install_executable(ethvm)
endif()
