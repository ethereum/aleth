# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

set(CMAKE_SYSTEM_PROCESSOR mips64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER mips64-linux-gnuabi64-gcc)
set(CMAKE_CXX_COMPILER mips64-linux-gnuabi64-g++)

set(CMAKE_FIND_ROOT_PATH /usr/mips64-linux-gnuabi64)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS Off)

if(${CMAKE_VERSION} VERSION_LESS 3.10.0)
    # Until CMake 3.10 the FindThreads uses try_run() check of -pthread flag,
    # what causes CMake error in crosscompiling mode. Avoid the try_run() check
    # by specifying the result up front.
    set(THREADS_PTHREAD_ARG TRUE)
endif()
