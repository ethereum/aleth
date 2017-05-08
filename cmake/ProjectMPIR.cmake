ExternalProject_Add(mpir
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME mpir-cmake.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/chfast/mpir/archive/cmake.tar.gz
    URL_HASH SHA256=2bf35ad7aa2b61bf4c2dcdab9aab48b2b4f9d6db8b937be4b0b5fe3b5efbdfa6
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Debug
        -DMPIR_GMP=On
)

ExternalProject_Get_Property(mpir INSTALL_DIR)
add_library(MPIR::mpir STATIC IMPORTED)
set(MPIR_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mpir${CMAKE_STATIC_LIBRARY_SUFFIX})
set(MPIR_INCLUDE_DIR ${INSTALL_DIR}/include)
set_property(TARGET MPIR::mpir PROPERTY IMPORTED_LOCATION ${MPIR_LIBRARY})
set_property(TARGET MPIR::mpir PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MPIR_INCLUDE_DIR})
add_dependencies(MPIR::mpir mpir)
unset(INSTALL_DIR)