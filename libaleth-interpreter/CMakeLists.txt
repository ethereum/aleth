# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

set(
    sources
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
)
add_library(aleth-interpreter STATIC ${sources})
target_link_libraries(aleth-interpreter PRIVATE devcore aleth-buildinfo evmc::evmc evmc::instructions)

if(ALETH_INTERPRETER_SHARED)
    # Build aleth-interpreter additionally as a shared library to include it in the package
    add_library(aleth-interpreter-shared SHARED ${sources})
    target_link_libraries(aleth-interpreter-shared PRIVATE devcore aleth-buildinfo evmc::evmc evmc::instructions)
    set_target_properties(aleth-interpreter-shared PROPERTIES OUTPUT_NAME aleth-interpreter)
    install(TARGETS aleth-interpreter-shared EXPORT alethTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(aleth-interpreter PRIVATE -fstack-usage)
endif()
