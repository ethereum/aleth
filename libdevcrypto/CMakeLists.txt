aux_source_directory(. SRC_LIST)
include_directories(..)
include_directories(../utils)

file(GLOB HEADERS "*.h")

add_library(devcrypto ${SRC_LIST} ${HEADERS})
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
eth_use(devcrypto REQUIRED Cryptopp)

find_package(Dev)
find_package(Utils)

target_link_libraries(devcrypto ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(devcrypto ${Utils_SCRYPT_LIBRARIES})
if (NOT EMSCRIPTEN)
	target_link_libraries(devcrypto ${Utils_SECP256K1_LIBRARIES})
endif ()

install(TARGETS devcrypto RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
