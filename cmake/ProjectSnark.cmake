include(ProjectMPIR)

# FIXME: Rename to LibFF as that's the name of the library.

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(SNARK_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ff${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SNARK_INCLUDE_DIR "${prefix}/include/libff")

ExternalProject_Add(snark
    PREFIX "${prefix}"
    DOWNLOAD_NAME libff-v2.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/chfast/libff/archive/cpp-ethereum-v2.tar.gz
    URL_HASH SHA256=f5a23730b9076938cf0b5ba1cb67f1f33873ca23dded58c52d7819c6b6c3ec91
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DGMP_INCLUDE_DIR=${MPIR_INCLUDE_DIR}
        -DGMP_LIBRARY=${MPIR_LIBRARY}
        -DCURVE=ALT_BN128 -DPERFORMANCE=Off -DWITH_PROCPS=Off
        -DUSE_PT_COMPRESSION=Off
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    LOG_BUILD 1
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
    BUILD_BYPRODUCTS "${SNARK_LIBRARY}"
)
add_dependencies(snark mpir)

# Create snark imported library
add_library(Snark STATIC IMPORTED)
file(MAKE_DIRECTORY ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Snark PROPERTY IMPORTED_LOCATION_RELEASE ${SNARK_LIBRARY})
set_property(TARGET Snark PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY INTERFACE_LINK_LIBRARIES MPIR::mpir)
add_dependencies(Snark snark)
