file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcrypto ${SOURCES} ${HEADERS})

find_package(Utils)
target_include_directories(devcrypto PRIVATE ..)
target_include_directories(devcrypto PRIVATE ../utils)
target_link_libraries(devcrypto Secp256k1 Cryptopp snark ${Utils_SCRYPT_LIBRARIES} devcore)
