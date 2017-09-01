# Setup ccache.
# ccache is auto-enabled if the tool is found. To disable set -DCCACHE=OFF option.
if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(CCACHE ccache)
    if(CCACHE)
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
        if(COTIRE_CMAKE_MODULE_VERSION)
            set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
        endif()
        message(STATUS "ccache enabled (${CCACHE})")
    endif()
endif()
