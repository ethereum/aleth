# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

if(cable_build_type_included)
    return()
endif()
set(cable_build_type_included TRUE)

macro(cable_set_build_type)
    if(NOT PROJECT_IS_NESTED)
        # Do this configuration only in the top project.
        if(NOT PROJECT_SOURCE_DIR)
            message(FATAL_ERROR "cable_set_build_type() can be used only after project()")
        endif()

        cmake_parse_arguments(build_type "" DEFAULT CONFIGURATION_TYPES ${ARGN})

        if(CMAKE_CONFIGURATION_TYPES)
            if(build_type_CONFIGURATION_TYPES)
                set(
                    CMAKE_CONFIGURATION_TYPES
                    ${build_type_CONFIGURATION_TYPES}
                    CACHE
                    STRING
                    "Available configurations for multi-configuration generators"
                    FORCE
                )
            endif()
            message(STATUS "[cable] Configurations: ${CMAKE_CONFIGURATION_TYPES}")
        else()
            if(build_type_DEFAULT AND NOT CMAKE_BUILD_TYPE)
                set(
                    CMAKE_BUILD_TYPE
                    ${build_type_DEFAULT}
                    CACHE STRING
                    "Build type for single-configuration generators"
                    FORCE
                )
            endif()
            message(STATUS "[cable] Build type: ${CMAKE_BUILD_TYPE}")
        endif()
    endif()
endmacro()
