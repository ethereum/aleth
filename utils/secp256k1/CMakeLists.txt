# This is copy of secp256k1 project from https://github.com/bitcoin-core/secp256k1
# Commit: https://github.com/bitcoin-core/secp256k1/commit/9d560f992db26612ce2630b194aef5f44d63a530
#
# It has been configured following official docs with following options:
#
# ./configure --disable-shared --disable-tests --disable-coverage --disable-openssl-tests --disable-exhaustive-tests --disable-jni --with-bignum=no --with-field=64bit --with-scalar=64bit --with-asm=no
#
# Build static context:
# make src/ecmult_static_context.h
#
# Copy src and include dirs.
#
# Copy CFLAGS from Makefile to target_compile_options().

# Reset definitions and includes.
# TODO: We should avoid using directory based configurations like add_definitions()
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES "")
set_property(DIRECTORY . PROPERTY COMPILE_DEFINITIONS "")
set_property(DIRECTORY . PROPERTY COMPILE_OPTIONS "")
set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "")
set(CMAKE_C_FLAGS_DEBUG "")

add_library(secp256k1 STATIC src/secp256k1.c)
target_compile_definitions(secp256k1 INTERFACE ETH_HAVE_SECP256K1 PRIVATE HAVE_CONFIG_H)
target_include_directories(secp256k1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(secp256k1 PRIVATE -O3 -W -std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings -fvisibility=hidden)


