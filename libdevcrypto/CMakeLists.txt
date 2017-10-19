file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcrypto ${SOURCES} ${HEADERS})
target_include_directories(devcrypto PRIVATE ../utils)
target_link_libraries(devcrypto PUBLIC devcore Secp256k1 PRIVATE libff::ff cryptopp-static scrypt)
