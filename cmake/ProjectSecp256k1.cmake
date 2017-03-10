include(ExternalProject)

if (MSVC)
    set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
    set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

ExternalProject_Add(secp256k1
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME secp256k1-9859f02f.tar.gz
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/chfast/secp256k1/archive/9859f02f9eca2787d697e7d11e5e9daf4c49732f.tar.gz
    URL_HASH SHA256=d31e40f3bdc2982d2e21f21e1e7a50731bae258f5245d51e32481bbe549035c4
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_LIST_DIR}/secp256k1/CMakeLists.txt <SOURCE_DIR>
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
               ${_only_release_configuration}
    LOG_CONFIGURE 1
    BUILD_COMMAND ""
    ${_overwrite_install_command}
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
