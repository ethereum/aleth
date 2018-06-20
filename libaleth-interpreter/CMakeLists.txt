# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

if(ALETH_INTERPRETER_SHARED)
    set(library_type SHARED)
else()
    set(library_type STATIC)
endif()

add_library(
    aleth-interpreter
    ${library_type}
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
)
target_link_libraries(aleth-interpreter PRIVATE devcore aleth-buildinfo evmc::evmc evmc::instructions)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(aleth-interpreter PRIVATE -fstack-usage)
endif()
