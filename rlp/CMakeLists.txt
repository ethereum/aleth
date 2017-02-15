aux_source_directory(. SRC_LIST)

add_executable(rlp ${SRC_LIST})

if (NOT EMSCRIPTEN AND NOT MSVC)
	eth_use(rlp ${REQUIRED} Utils::secp256k1)
endif()

find_package(Dev)

target_include_directories(rlp PRIVATE ..)
target_include_directories(rlp PRIVATE ../utils)
target_link_libraries(rlp ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(rlp ${Dev_DEVCRYPTO_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(rlp pthread)
endif()

