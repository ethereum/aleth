# This CMake config file for secp256k1 project from https://github.com/bitcoin-core/secp256k1
#
# The secp256k1 project has been configured following official docs with following options:
#
# ./configure --disable-shared --disable-tests --disable-coverage --disable-openssl-tests --disable-exhaustive-tests --disable-jni --with-bignum=no --with-field=64bit --with-scalar=64bit --with-asm=no
#
# Build static context:
# make src/ecmult_static_context.h
#
# Copy src/ecmult_static_context.h and src/libsecp256k1-config.h
#
# Copy CFLAGS from Makefile to COMPILE_OPTIONS.

cmake_minimum_required(VERSION 3.4)
project(secp256k1 LANGUAGES C)

if (NOT MSVC)
	# Use default Release flags on MSVC.
	set(COMPILE_OPTIONS -O3 -W -std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings -fvisibility=hidden)
endif()

add_library(secp256k1 STATIC src/secp256k1.c)
target_compile_definitions(secp256k1 PRIVATE HAVE_CONFIG_H)
target_include_directories(secp256k1 PRIVATE . src)
target_compile_options(secp256k1 PRIVATE ${COMPILE_OPTIONS})
install(TARGETS secp256k1 ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
