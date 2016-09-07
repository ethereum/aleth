aux_source_directory(. SRC_LIST)

add_executable(rlp ${SRC_LIST})

eth_use(rlp REQUIRED Cryptopp)

if (NOT EMSCRIPTEN AND NOT MSVC)
	eth_use(rlp ${REQUIRED} Utils::secp256k1)
endif()

find_package(Dev)

target_include_directories(rlp PRIVATE ..)
target_include_directories(rlp PRIVATE ../utils)
target_link_libraries(rlp ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(rlp ${Dev_DEVCRYPTO_LIBRARIES})

if (MSVC)
	find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
	target_link_libraries(rlp ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(rlp ${Boost_DATE_TIME_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(rlp pthread)
endif()

eth_install_executable(rlp)

