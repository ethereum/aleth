include(ExternalProject)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(JSONCPP_CMAKE_COMMAND emcmake cmake)
else()
    set(JSONCPP_CMAKE_COMMAND ${CMAKE_COMMAND})
endif()

if (MSVC)
    set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
    set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

ExternalProject_Add(jsoncpp
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME jsoncpp-1.7.7.tar.gz
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/open-source-parsers/jsoncpp/archive/1.7.7.tar.gz
    URL_HASH SHA256=087640ebcf7fbcfe8e2717a0b9528fff89c52fcf69fa2a18cc2b538008098f97
    CMAKE_COMMAND ${JSONCPP_CMAKE_COMMAND}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               # Build static lib but suitable to be included in a shared lib.
               -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
               ${_only_release_configuration}
               -DJSONCPP_WITH_TESTS=Off
               -DJSONCPP_WITH_PKGCONFIG_SUPPORT=Off
    LOG_CONFIGURE 1
    BUILD_COMMAND ""
    ${_overwrite_install_command}
    LOG_INSTALL 1
)

# Create jsoncpp imported library
ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
add_library(JsonCpp STATIC IMPORTED)
set(JSONCPP_LIBRARY ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jsoncpp${CMAKE_STATIC_LIBRARY_SUFFIX})
set(JSONCPP_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${JSONCPP_INCLUDE_DIR})  # Must exist.
set_property(TARGET JsonCpp PROPERTY IMPORTED_LOCATION ${JSONCPP_LIBRARY})
set_property(TARGET JsonCpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${JSONCPP_INCLUDE_DIR})
add_dependencies(JsonCpp jsoncpp)
unset(INSTALL_DIR)
