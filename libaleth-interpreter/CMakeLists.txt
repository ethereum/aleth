# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

add_library(
    aleth-interpreter
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
    VMSIMD.cpp
    VMValidate.cpp
)
target_link_libraries(aleth-interpreter PRIVATE devcore aleth-buildinfo evmc::evmc evmc::instructions)
