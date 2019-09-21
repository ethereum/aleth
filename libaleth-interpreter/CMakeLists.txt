# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018-2019 Aleth Authors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.
hunter_add_package(intx)
find_package(intx CONFIG REQUIRED)

set(
    sources
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
)

set(
    dependencies
    aleth-buildinfo
    ethash::ethash
    evmc::evmc
    evmc::instructions
    Boost::boost
    intx::intx
)

add_library(aleth-interpreter STATIC ${sources})
target_link_libraries(aleth-interpreter PRIVATE ${dependencies})

if(EVM_OPTIMIZE)
    target_compile_definitions(aleth-interpreter PRIVATE EVM_OPTIMIZE)
endif()

if(ALETH_INTERPRETER_SHARED)
    # Build aleth-interpreter additionally as a shared library to include it in the package
    add_library(aleth-interpreter-shared SHARED ${sources})
    target_link_libraries(aleth-interpreter-shared PRIVATE ${dependencies})
    set_target_properties(aleth-interpreter-shared PROPERTIES OUTPUT_NAME aleth-interpreter)
    install(TARGETS aleth-interpreter-shared EXPORT alethTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    if(EVM_OPTIMIZE)
        target_compile_definitions(aleth-interpreter-shared PRIVATE EVM_OPTIMIZE)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(aleth-interpreter PRIVATE -fstack-usage)
endif()
