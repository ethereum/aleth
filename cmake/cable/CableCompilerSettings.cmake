# Cable: CMake Bootstrap Library
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

include(CheckCXXCompilerFlag)

# Adds CXX compiler flag if the flag is supported by the compiler.
#
# This is effectively a combination of CMake's check_cxx_compiler_flag()
# and add_compile_options():
#
#    if(check_cxx_compiler_flag(flag))
#        add_compile_options(flag)
#
function(cable_add_cxx_compiler_flag_if_supported FLAG)
    # Remove leading - or / from the flag name.
    string(REGEX REPLACE "^-|/" "" name ${FLAG})
    check_cxx_compiler_flag(${FLAG} ${name})
    if(${name})
        add_compile_options(${FLAG})
    endif()

    # If the optional argument passed, store the result there.
    if(ARGV1)
        set(${ARGV1} ${name} PARENT_SCOPE)
    endif()
endfunction()


# Configures the compiler with default flags.
macro(cable_configure_compiler)
    if(NOT PROJECT_IS_NESTED)
        # Do this configuration only in the top project.

        cmake_parse_arguments(cable "NO_STACK_PROTECTION" "" "" ${ARGN})

        if(cable_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "cable_configure_compiler: Uknown options: ${cable_UNPARSED_ARGUMENTS}")
        endif()

        # Set helper variables recognizing C++ compilers.
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
            set(CABLE_COMPILER_GNU TRUE)
        elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
            # This matches both clang and AppleClang.
            set(CABLE_COMPILER_CLANG TRUE)
        endif()

        if(CABLE_COMPILER_GNU OR CABLE_COMPILER_CLANG)
            set(CABLE_COMPILER_GNULIKE TRUE)
        endif()

        if(CABLE_COMPILER_GNULIKE)

            # Enable basing warnings set and treat them as errors.
            add_compile_options(-Wall -Wextra -Werror -pedantic)

            # Allow unknown pragmas, we don't want to wrap them with #ifdefs.
            add_compile_options(-Wno-unknown-pragmas)

        elseif(MSVC)

            # Enable basing warnings set and treat them as errors.
            add_compile_options(/W4 /WX)

            # Allow unknown pragmas, we don't want to wrap them with #ifdefs.
            add_compile_options(/wd4068)

        endif()

        check_cxx_compiler_flag(-fstack-protector fstack-protector)
        if(fstack-protector)
            # The compiler supports stack protection options.
            if(cable_NO_STACK_PROTECTION)
                # Stack protection explicitly disabled.
                # Add "no" flag, because in some configuration the compiler has it enabled by default.
                add_compile_options(-fno-stack-protector)
            else()
                # Try enabling the "strong" variant.
                cable_add_cxx_compiler_flag_if_supported(-fstack-protector-strong have_stack_protector_strong_support)
                if(NOT have_stack_protector_strong_support)
                    # Fallback to standard variant of "strong" not available.
                    add_compile_options(-fstack-protector)
                endif()
            endif()
        endif()

        cable_add_cxx_compiler_flag_if_supported(-Wimplicit-fallthrough)

        # Sanitizers support.
        set(SANITIZE OFF CACHE STRING "Build with the specified sanitizer")
        if(SANITIZE)
            # Set the linker flags first, they are required to properly test the compiler flag.
            set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_SHARED_LINKER_FLAGS}")
            set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_EXE_LINKER_FLAGS}")

            set(test_name have_fsanitize_${SANITIZE})
            check_cxx_compiler_flag(-fsanitize=${SANITIZE} ${test_name})
            if(NOT ${test_name})
                message(FATAL_ERROR "Unsupported sanitizer: ${SANITIZE}")
            endif()
            add_compile_options(-fno-omit-frame-pointer -fsanitize=${SANITIZE})

            set(backlist_file ${PROJECT_SOURCE_DIR}/sanitizer-blacklist.txt)
            if(EXISTS ${backlist_file})
                check_cxx_compiler_flag(-fsanitize-blacklist=${backlist_file} have_fsanitize-blacklist)
                if(have_fsanitize-blacklist)
                    add_compile_options(-fsanitize-blacklist=${backlist_file})
                endif()
            endif()
        endif()

        # Code coverage support.
        option(COVERAGE "Build with code coverage support" OFF)
        if(COVERAGE)
            # Set the linker flags first, they are required to properly test the compiler flag.
            set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
            set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
            check_cxx_compiler_flag(--coverage have_coverage)
            if(NOT have_coverage)
                message(FATAL_ERROR "Unsupported sanitizer: ${SANITIZE}")
            endif()
            add_compile_options(-g --coverage)
        endif()

    endif()
endmacro()
