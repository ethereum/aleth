add_compile_options(-Wno-unused-parameters)

file(GLOB sources "*.c")
file(GLOB headers "*.h")

add_library(hacl ${sources} ${headers})

cmake_minimum_required(VERSION 3.2)

# Project Name
project (hacl)

# Build settings
set(CMAKE_BUILD_TYPE Release)

# Verified Files
set(SOURCE_FILES_VERIFIED
   kremlib.c
   FStar.c
   Hacl_Policies.c
   AEAD_Poly1305_64.c
   Hacl_Chacha20.c
   Hacl_Chacha20Poly1305.c
   Hacl_Curve25519.c
   Hacl_Ed25519.c
   Hacl_Poly1305_64.c
   Hacl_SHA2_256.c
   Hacl_SHA2_384.c
   Hacl_SHA2_512.c
   Hacl_HMAC_SHA2_256.c
   Hacl_Salsa20.c
   NaCl.c)

# Experimental Files
set(SOURCE_FILES_EXPERIMENTAL
   cpuid.c
   drng.c
   Random.c)

# Define a user variable to determinate if experimental files are build
option(Experimental "Include experimental code in HaCl build" OFF)

# Final set of files to build the libraries upon
if (Experimental)
   set(SOURCE_FILES ${SOURCE_FILES_VERIFIED} ${SOURCE_FILES_EXPERIMENTAL})
else ()
   set(SOURCE_FILES ${SOURCE_FILES_VERIFIED})
endif ()

# Compilation options
set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")

# Generate both a static and a shared library
add_library(hacl_static STATIC ${SOURCE_FILES})
set_target_properties(hacl_static PROPERTIES OUTPUT_NAME hacl)

add_library(hacl_shared SHARED ${SOURCE_FILES})
set_target_properties(hacl_shared PROPERTIES OUTPUT_NAME hacl)

# Note: on Windows, depending on the build system,
#       both static and shared can have the .lib extension
#       (You can change the OUTPUT_NAME in that case...)
