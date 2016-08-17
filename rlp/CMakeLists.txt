aux_source_directory(. SRC_LIST)
include_directories(..)
include_directories(../utils)

add_executable(rlp ${SRC_LIST})

eth_use(rlp REQUIRED Cryptopp)

if ((NOT EMSCRIPTEN) AND (NOT DEFINED MSVC))
	eth_use(rlp ${REQUIRED} Utils::secp256k1)
endif()


find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
find_package(Dev)

target_link_libraries(rlp ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(rlp ${Dev_DEVCRYPTO_LIBRARIES})
if (DEFINED MSVC)
	target_link_libraries(rlp ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(rlp ${Boost_DATE_TIME_LIBRARIES})
endif()
if (UNIX AND NOT APPLE)
	target_link_libraries(rlp pthread)
endif()

if (APPLE)
	install(TARGETS rlp DESTINATION bin)
else()
	eth_install_executable(rlp)
endif()

