include("${CMAKE_CURRENT_LIST_DIR}/helpers.cmake")

set(INSTALL_DIR "${ROOT_DIR}/deps")
set(SERVER "https://github.com/ethereum/cpp-dependencies/releases/download/vc140/")

function(download_and_install PACKAGE_NAME)
    download_and_unpack("${SERVER}${PACKAGE_NAME}.tar.gz" ${INSTALL_DIR})
endfunction(download_and_install)


download_and_install("curl-7.4.2")
download_and_install("leveldb-1.2")
download_and_install("microhttpd-0.9.2")
download_and_install("miniupnpc-1.9")
