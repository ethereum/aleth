include(ProjectMPIR)

# FIXME: Rename to LibFF as that's the name of the library.

ExternalProject_Add(snark
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME libff-a1021d20.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/chfast/libff/archive/a1021d2066d1b73fe701fdb60b2e521134b126bf.tar.gz
    URL_HASH SHA256=f71052cd97e55898853f44434dbf3a804d3b66afd950a81093a277bcc9e002f6
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DGMP_INCLUDE_DIR=${MPIR_INCLUDE_DIR}
        -DCURVE=ALT_BN128 -DPERFORMANCE=Off -DWITH_PROCPS=Off
        -DUSE_PT_COMPRESSION=Off
)
add_dependencies(snark boost)

# Create snark imported library
ExternalProject_Get_Property(snark INSTALL_DIR)
add_library(Snark STATIC IMPORTED)
set(SNARK_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ff${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SNARK_INCLUDE_DIR ${INSTALL_DIR}/include/libff)
file(MAKE_DIRECTORY ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY IMPORTED_LOCATION ${SNARK_LIBRARY})
set_property(TARGET Snark PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY INTERFACE_LINK_LIBRARIES MPIR::mpz)
add_dependencies(Snark snark)
unset(INSTALL_DIR)
