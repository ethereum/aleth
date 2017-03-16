# Depends on ProjectBoost.cmake

# FIXME: Rename to LibFF as that's the name of the library.

ExternalProject_Add(snark
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME libff-2fa434b3.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/scipr-lab/libff/archive/2fa434b3d6e9163beedaefffa85043a1180a05e6.tar.gz
    URL_HASH SHA256=d3266f95a86bbfc908a7a9531df08a47cdd026e76b6290465fb8dfa203606a21

    PATCH_COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/third_party/gtest/CMakeLists.txt
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DBOOST_INCLUDEDIR=${BOOST_INCLUDE_DIR}
        -DBOOST_LIBRARYDIR=${BOOST_LIBRARY_DIR}
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
set_property(TARGET Snark PROPERTY INTERFACE_LINK_LIBRARIES gmp)
add_dependencies(Snark snark)
unset(INSTALL_DIR)
