aux_source_directory(. SRC_LIST)

add_executable(bench ${SRC_LIST})

find_package(Dev)

target_include_directories(bench PRIVATE ..)
target_include_directories(bench PRIVATE ../utils)
target_link_libraries(bench ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(bench ${Dev_DEVCRYPTO_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(bench pthread)
endif()
