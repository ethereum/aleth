# Cable: CMake Bootstrap Library.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

# Bootstrap the Cable - CMake Boostrap Library by including this file.
# e.g. include(cmake/cable/bootstrap.cmake).


# Cable version.
#
# This is internal variable automaticaly updated with external tools.
# Use CABLE_VERSION variable if you need this information.
set(version 0.1.5)


# For conveniance, add the parent dir containg CMake modules to module path.
get_filename_component(module_dir ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
list(APPEND CMAKE_MODULE_PATH ${module_dir})
unset(module_dir)


if(CABLE_VERSION)
    # Some other instance of Cable was initialized in the top project.

    # Compare versions of the top project and this instances.
    if(CABLE_VERSION VERSION_LESS version)
        set(severity WARNING)
        set(comment "vesion older than ${version}")
    elseif(CABLE_VERSION VERSION_EQUAL version)
        set(severity STATUS)
        set(comment "same version")
    else()
        set(severity STATUS)
        set(comment "version newer than ${version}")
    endif()

    # Find the name of the top project.
    # Make sure the name is not overwritten by multiple nested projects.
    if(NOT DEFINED cable_top_project_name)
        set(cable_top_project_name ${PROJECT_NAME})
    endif()

    # Mark this project as nested.
    set(PROJECT_IS_NESTED TRUE)

    message(
        ${severity}
        "[cable] Parent Cable ${CABLE_VERSION} (${comment}) initialized in `${cable_top_project_name}` project"
    )

    unset(version)
    unset(severity)
    unset(comment)
    return()
endif()

# Export Cable version.
set(CABLE_VERSION ${version})

# Mark this project as non-nested.
set(PROJECT_IS_NESTED FALSE)

# Add Cable modules to the CMake module path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "[cable] Cable ${CABLE_VERSION} initialized")

unset(version)
