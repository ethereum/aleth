cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

aux_source_directory(. SRC_LIST)

set(EXECUTABLE devcrypto)

file(GLOB HEADERS "*.h")
add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Eth::devcore Eth::scrypt Cryptopp)

if (NOT DEFINED MSVC)
	eth_use(${EXECUTABLE} REQUIRED Eth::secp256k1)
endif ()

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

