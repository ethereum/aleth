aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(devcrypto ${SRC_LIST} ${HEADERS})
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

find_package(Dev)
find_package(Utils)

target_include_directories(devcrypto PRIVATE ..)
target_include_directories(devcrypto PRIVATE ../utils)
target_link_libraries(devcrypto cryptopp ${Utils_SCRYPT_LIBRARIES} devcore)

if (NOT EMSCRIPTEN)
	target_link_libraries(devcrypto ${Utils_SECP256K1_LIBRARIES})
endif ()
