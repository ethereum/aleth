
file(GLOB HEADERS "*.h")

add_library(secp256k1 STATIC secp256k1.c include/secp256k1.h ${HEADERS})
target_compile_definitions(secp256k1 INTERFACE ETH_HAVE_SECP256K1 PRIVATE HAVE_CONFIG_H)

if (NOT MSVC)
	include(EthDependencies)
	eth_use(secp256k1 REQUIRED Gmp)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -W -std=c89 -fPIC -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings")
endif()
