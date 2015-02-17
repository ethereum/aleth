# Install script for directory: /Users/alex/dev/cpp-ethereum/libweb3jsonrpc

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libweb3jsonrpc/Debug/libweb3jsonrpc.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libweb3jsonrpc.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Debug/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/Debug/libdevcrypto.dylib" "libdevcrypto.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethcore/Debug/libethcore.dylib" "libethcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethereum/Debug/libethereum.dylib" "libethereum.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevm/Debug/libevm.dylib" "libevm.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/Debug/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/Debug/liblll.dylib" "liblll.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/Debug/libp2p.dylib" "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libserpent/Debug/libserpent.dylib" "libserpent.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libsolidity/Debug/libsolidity.dylib" "libsolidity.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwebthree/Debug/libwebthree.dylib" "libwebthree.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwhisper/Debug/libwhisper.dylib" "libwhisper.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/secp256k1/Debug/libsecp256k1.dylib" "libsecp256k1.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libweb3jsonrpc/Release/libweb3jsonrpc.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libweb3jsonrpc.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Release/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/Release/libdevcrypto.dylib" "libdevcrypto.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethcore/Release/libethcore.dylib" "libethcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethereum/Release/libethereum.dylib" "libethereum.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevm/Release/libevm.dylib" "libevm.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/Release/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/Release/liblll.dylib" "liblll.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/Release/libp2p.dylib" "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libserpent/Release/libserpent.dylib" "libserpent.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libsolidity/Release/libsolidity.dylib" "libsolidity.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwebthree/Release/libwebthree.dylib" "libwebthree.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwhisper/Release/libwhisper.dylib" "libwhisper.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/secp256k1/Release/libsecp256k1.dylib" "libsecp256k1.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libweb3jsonrpc/MinSizeRel/libweb3jsonrpc.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libweb3jsonrpc.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/MinSizeRel/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/MinSizeRel/libdevcrypto.dylib" "libdevcrypto.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethcore/MinSizeRel/libethcore.dylib" "libethcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethereum/MinSizeRel/libethereum.dylib" "libethereum.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevm/MinSizeRel/libevm.dylib" "libevm.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/MinSizeRel/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/MinSizeRel/liblll.dylib" "liblll.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/MinSizeRel/libp2p.dylib" "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libserpent/MinSizeRel/libserpent.dylib" "libserpent.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libsolidity/MinSizeRel/libsolidity.dylib" "libsolidity.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwebthree/MinSizeRel/libwebthree.dylib" "libwebthree.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwhisper/MinSizeRel/libwhisper.dylib" "libwhisper.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/secp256k1/MinSizeRel/libsecp256k1.dylib" "libsecp256k1.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libweb3jsonrpc/RelWithDebInfo/libweb3jsonrpc.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libweb3jsonrpc.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/RelWithDebInfo/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/RelWithDebInfo/libdevcrypto.dylib" "libdevcrypto.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethcore/RelWithDebInfo/libethcore.dylib" "libethcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libethereum/RelWithDebInfo/libethereum.dylib" "libethereum.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevm/RelWithDebInfo/libevm.dylib" "libevm.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/RelWithDebInfo/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/RelWithDebInfo/liblll.dylib" "liblll.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/RelWithDebInfo/libp2p.dylib" "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libserpent/RelWithDebInfo/libserpent.dylib" "libserpent.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libsolidity/RelWithDebInfo/libsolidity.dylib" "libsolidity.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwebthree/RelWithDebInfo/libwebthree.dylib" "libwebthree.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libwhisper/RelWithDebInfo/libwhisper.dylib" "libwhisper.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/secp256k1/RelWithDebInfo/libsecp256k1.dylib" "libsecp256k1.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libweb3jsonrpc.dylib")
      endif()
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/web3jsonrpc" TYPE FILE FILES
    "/Users/alex/dev/cpp-ethereum/libweb3jsonrpc/abstractwebthreestubserver.h"
    "/Users/alex/dev/cpp-ethereum/libweb3jsonrpc/WebThreeStubServer.h"
    "/Users/alex/dev/cpp-ethereum/libweb3jsonrpc/WebThreeStubServerBase.h"
    )
endif()

