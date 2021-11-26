#------------------------------------------------------------------------------
# EthCompilerSettings.cmake
#
# CMake file for cpp-ethereum project which specifies our compiler settings
# for each supported platform and build configuration.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# Copyright (c) 2014-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

# Clang seeks to be command-line compatible with GCC as much as possible, so
# most of our compiler settings are common between GCC and Clang.
#
# These settings then end up spanning all POSIX platforms (Linux, OS X, BSD, etc)

if (CABLE_COMPILER_GNULIKE)
    # Configuration-specific compiler settings.
    set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -DETH_DEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

    # Hide all symbols by default.
    add_compile_options(-fvisibility=hidden)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Do not export symbols from dependencies.
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
    endif()

    # Check GCC compiler version.
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 5)
            message(FATAL_ERROR "This compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} is not supported. GCC 5 or newer is required.")
        endif()

    # Stop if buggy clang compiler detected.
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES AppleClang)
        if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 8.4)
            message(FATAL_ERROR "This compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} is not able to compile required libff. Install clang 4+ from Homebrew or XCode 9.")
        endif()
    endif()

# The major alternative compiler to GCC/Clang is Microsoft's Visual C++ compiler, only available on Windows.
elseif (MSVC)

    add_compile_options(/MP)                    # enable parallel compilation
    add_compile_options(/EHsc)                  # specify Exception Handling Model in msvc
    add_compile_options(/WX)                    # enable warnings-as-errors
    add_compile_options(/wd4068)                # disable unknown pragma warning (4068)
    add_compile_options(/wd4180)                # disable qualifier applied to function type has no meaning; ignored (4180)
    add_compile_options(/wd4244)                # disable conversion from 'type1' to 'type2', possible loss of data (4244)
    add_compile_options(/wd4267)                # disable conversion from 'size_t' to 'type', possible loss of data (4267)
    add_compile_options(/wd4290)                # disable C++ exception specification ignored except to indicate a function is not __declspec(nothrow) (4290)
    add_compile_options(/wd4297)                # disable <vector>'s function assumed not to throw an exception but does (4297)
    add_compile_options(/wd4503)                # disable decorated name length exceeded, name was truncated (4503)
    add_compile_options(/wd4714)                # disable function marked as __forceinline not inlined
    add_compile_options(/wd4800)                # disable forcing value to bool 'true' or 'false' (performance warning) (4800)
    add_compile_options(/wd4996)                # disable unsafe function warning (4996)
    add_compile_options(-D_WIN32_WINNT=0x0600)  # declare Windows Vista API requirement
    add_compile_options(-DNOMINMAX)             # undefine windows.h MAX && MIN macros cause it cause conflicts with std::min && std::max functions
    add_compile_options(-DMINIUPNP_STATICLIB)   # define miniupnp static library

    # Always use Release variant of C++ runtime.
    # We don't want to provide Debug variants of all dependencies. Some default
    # flags set by CMake must be tweaked.
    string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MDd" "/MD" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/D_DEBUG" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS OFF)

    # disable empty object file warning
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
    # warning LNK4075: ignoring '/EDITANDCONTINUE' due to '/SAFESEH' specification
    # warning LNK4099: pdb was not found with lib
    # stack size 16MB
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099,4075 /STACK:16777216")

# If you don't have GCC, Clang or VC++ then you are on your own.  Good luck!
else ()
    message(WARNING "Your compiler is not tested, if you run into any issues, we'd welcome any patches.")
endif ()

if(UNIX AND NOT APPLE)
    option(STATIC_LIBSTDCPP "Link libstdc++ staticly")
    if(STATIC_LIBSTDCPP)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
    endif()
endif()
