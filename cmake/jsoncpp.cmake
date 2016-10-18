include(ExternalProject)

ExternalProject_Add(jsoncpp-project
    PREFIX deps/jsoncpp
    URL https://github.com/open-source-parsers/jsoncpp/archive/1.7.7.tar.gz
    URL_HASH SHA256=087640ebcf7fbcfe8e2717a0b9528fff89c52fcf69fa2a18cc2b538008098f97
    DOWNLOAD_NO_PROGRESS True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               # Build static lib but suitable to be included in a shared lib.
               -DCMAKE_POSITION_INDEPENDENT_CODE=On
               -DJSONCPP_WITH_TESTS=Off
               -DJSONCPP_WITH_PKGCONFIG_SUPPORT=Off
    # Overwtire build and install commands to force Release build on MSVC.
    BUILD_COMMAND cmake --build <BINARY_DIR> --config Release
    INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install
)

# Create jsoncpp imported library
ExternalProject_Get_Property(jsoncpp-project INSTALL_DIR)
add_library(jsoncpp STATIC IMPORTED)
set(JSONCPP_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jsoncpp${CMAKE_STATIC_LIBRARY_SUFFIX})
set(JSONCPP_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${JSONCPP_INCLUDE_DIR})  # Must exist.
set_property(TARGET jsoncpp PROPERTY IMPORTED_LOCATION ${JSONCPP_LIBRARY})
set_property(TARGET jsoncpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${JSONCPP_INCLUDE_DIR})
add_dependencies(jsoncpp jsoncpp-project)
unset(INSTALL_DIR)
