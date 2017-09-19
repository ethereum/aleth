file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcrypto ${SOURCES} ${HEADERS})
target_include_directories(devcrypto PRIVATE ../utils)
target_link_libraries(devcrypto PUBLIC devcore PRIVATE Secp256k1 Snark cryptopp-static scrypt)
