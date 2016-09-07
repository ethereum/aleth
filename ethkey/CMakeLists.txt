aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_executable(ethkey ${SRC_LIST} ${HEADERS})

find_package(Eth)

target_include_directories(ethkey PRIVATE ..)
target_link_libraries(ethkey ${Eth_ETHCORE_LIBRARIES})

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(ethkey ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethkey ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(ethkey pthread)
endif()

eth_install_executable(ethkey)
