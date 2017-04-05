include(ExternalProject)

ExternalProject_Add(lcov
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/linux-test-project/lcov/releases/download/v1.13/lcov-1.13.tar.gz
    URL_HASH SHA256=44972c878482cc06a05fe78eaa3645cbfcbad6634615c3309858b207965d8a23
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin <INSTALL_DIR>/bin
)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/bin)
set(LCOV_TOOL ${CMAKE_SOURCE_DIR}/deps/bin/lcov)
