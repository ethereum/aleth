if(ProjectBinaryenIncluded)
    return()
endif()
set(ProjectBinaryenIncluded TRUE)

include(ExternalProject)

if(MSVC)
    # Overwrite build and install commands to force Release build on MSVC.
    set(build_command BUILD_COMMAND cmake --build <BINARY_DIR> --config Release)
    set(install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
elseif(CMAKE_GENERATOR STREQUAL Ninja)
    # For Ninja we have to pass the number of jobs from CI environment.
    # Otherwise it will go crazy and run out of memory.
    if($ENV{BUILD_PARALLEL_JOBS})
        set(build_command BUILD_COMMAND cmake --build <BINARY_DIR> -- -j $ENV{BUILD_PARALLEL_JOBS})
        message(STATUS "Ninja $ENV{BUILD_PARALLEL_JOBS}")
    endif()
endif()

set(prefix ${CMAKE_BINARY_DIR}/deps)
set(source_dir ${prefix}/src/binaryen)
set(binary_dir ${prefix}/src/binaryen-build)
# Use source dir because binaryen only installs single header with C API.
set(binaryen_include_dir ${source_dir}/src)
set(binaryen_library ${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}binaryen${CMAKE_STATIC_LIBRARY_SUFFIX})
# Include also other static libs needed:
set(binaryen_other_libraries
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}wasm${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}asmjs${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}passes${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cfg${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ir${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}emscripten-optimizer${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}support${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(BINARYEN_VERSION 1.37.35)

ExternalProject_Add(binaryen
    PREFIX ${prefix}
    DOWNLOAD_NAME binaryen-${BINARYEN_VERSION}.tar.gz
    DOWNLOAD_DIR ${prefix}/downloads
    SOURCE_DIR ${source_dir}
    BINARY_DIR ${binary_dir}
    URL https://github.com/WebAssembly/binaryen/archive/${BINARYEN_VERSION}.tar.gz
    URL_HASH SHA256=19439e41dc576446eaae0c4a8e07d4cd4c40aea7dfb0a6475b925686852f8006
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_STATIC_LIB=ON
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    ${build_command}
    ${install_command}
    BUILD_BYPRODUCTS ${binaryen_library} ${binaryen_other_libraries}
)

add_library(binaryen::binaryen STATIC IMPORTED)

file(MAKE_DIRECTORY ${binaryen_include_dir})  # Must exist.
set_target_properties(
    binaryen::binaryen
    PROPERTIES
    IMPORTED_CONFIGURATIONS Release
    IMPORTED_LOCATION_RELEASE ${binaryen_library}
    INTERFACE_INCLUDE_DIRECTORIES ${binaryen_include_dir}
    INTERFACE_LINK_LIBRARIES "${binaryen_other_libraries}"

)

add_dependencies(binaryen::binaryen binaryen)
