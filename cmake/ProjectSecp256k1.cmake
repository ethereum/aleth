include(ExternalProject)

if (MSVC)
    set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
endif()

ExternalProject_Add(secp256k1
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME secp256k1-9d560f99.tar.gz
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/bitcoin-core/secp256k1/archive/9d560f992db26612ce2630b194aef5f44d63a530.tar.gz
    URL_HASH SHA256=910b2535bdbb5d235e9212c92050e0f9b327e05129b449f8f3d3c6d558121284
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/secp256k1
        <SOURCE_DIR>
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
               ${_only_release_configuration}
    LOG_CONFIGURE 1
    LOG_INSTALL 1
)

# Create imported library
ExternalProject_Get_Property(secp256k1 INSTALL_DIR)
add_library(Secp256k1 STATIC IMPORTED)
set(SECP256K1_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}secp256k1${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SECP256K1_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${SECP256K1_INCLUDE_DIR})  # Must exist.
set_property(TARGET Secp256k1 PROPERTY IMPORTED_LOCATION ${SECP256K1_LIBRARY})
set_property(TARGET Secp256k1 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SECP256K1_INCLUDE_DIR})
add_dependencies(Secp256k1 secp256k1)
unset(INSTALL_DIR)
