aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_executable(ethkey ${SRC_LIST} ${HEADERS})

find_package(Eth)

target_include_directories(ethkey PRIVATE ..)
target_link_libraries(ethkey ${Eth_ETHCORE_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(ethkey pthread)
endif()
