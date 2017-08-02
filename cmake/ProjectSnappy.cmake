if(NOT MSVC)
    set(snappy_cxx_flags '-std=c++11')  # Needed for ABI compatibility for std::string.
endif()

ExternalProject_Add(snappy
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME snappy-1.1.6.tar.gz
    DOWNLOAD_NO_PROGRESS TRUE
    URL https://github.com/google/snappy/archive/1.1.6.tar.gz
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_CXX_FLAGS=${snappy_cxx_flags}
)

ExternalProject_Get_Property(snappy INSTALL_DIR)
add_library(Snappy SHARED IMPORTED)
set(SNAPPY_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}snappy${CMAKE_SHARED_LIBRARY_SUFFIX})
set(SNAPPY_INCLUDE_DIR ${INSTALL_DIR}/include)
set_property(TARGET Snappy PROPERTY IMPORTED_LOCATION ${SNAPPY_LIBRARY})
set_property(TARGET Snappy PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SNAPPY_INCLUDE_DIR})
add_dependencies(Snappy snappy)
unset(INSTALL_DIR)