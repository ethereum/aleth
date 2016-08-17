aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

add_executable(ethkey ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
find_package(Eth)

if (DEFINED MSVC)
	target_link_libraries(ethkey ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethkey ${Boost_DATE_TIME_LIBRARIES})
endif()
target_link_libraries(ethkey ${Eth_ETHCORE_LIBRARIES})
if (UNIX AND NOT APPLE)
	target_link_libraries(ethkey pthread)
endif()

if (APPLE)
	install(TARGETS ethkey DESTINATION bin)
else()
	eth_install_executable(ethkey)
endif()
