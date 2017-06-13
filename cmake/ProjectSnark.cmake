include(ProjectMPIR)

# FIXME: Rename to LibFF as that's the name of the library.

ExternalProject_Add(snark
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME libff-97d3fa6c.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/chfast/libff/archive/97d3fa6cdbd4b7549c7a8a179dc97308dbfd044d.tar.gz
    URL_HASH SHA256=f102f3ee43c96c9a81c20d8c0446c805c6b8c0e3121518b3625f08e2c230096e
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DGMP_INCLUDE_DIR=${MPIR_INCLUDE_DIR}
        -DGMP_LIBRARIES=${MPIR_LIBRARY}
        -DGMPXX_LIBRARIES=${MPIR_LIBRARY}
        -DCURVE=ALT_BN128 -DPERFORMANCE=Off -DWITH_PROCPS=Off
        -DUSE_PT_COMPRESSION=Off
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
)
add_dependencies(snark boost)
add_dependencies(snark mpir)

# Create snark imported library
ExternalProject_Get_Property(snark INSTALL_DIR)
add_library(Snark STATIC IMPORTED)
set(SNARK_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ff${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SNARK_INCLUDE_DIR ${INSTALL_DIR}/include/libff)
file(MAKE_DIRECTORY ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY IMPORTED_LOCATION ${SNARK_LIBRARY})
set_property(TARGET Snark PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SNARK_INCLUDE_DIR})
set_property(TARGET Snark PROPERTY INTERFACE_LINK_LIBRARIES MPIR::mpir)
add_dependencies(Snark snark)
unset(INSTALL_DIR)
