# Install script for directory: /Users/alex/dev/cpp-ethereum/libp2p

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
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/Debug/libp2p.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Debug/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/Debug/libdevcrypto.dylib" "libdevcrypto.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/Release/libp2p.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Release/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/Release/libdevcrypto.dylib" "libdevcrypto.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/MinSizeRel/libp2p.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/MinSizeRel/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/MinSizeRel/libdevcrypto.dylib" "libdevcrypto.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/RelWithDebInfo/libp2p.dylib")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -id "libp2p.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/RelWithDebInfo/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/RelWithDebInfo/libdevcrypto.dylib" "libdevcrypto.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libp2p.dylib")
      endif()
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/p2p" TYPE FILE FILES
    "/Users/alex/dev/cpp-ethereum/libp2p/All.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Capability.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Common.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Host.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/HostCapability.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Network.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/NodeTable.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Peer.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/Session.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/UDP.h"
    "/Users/alex/dev/cpp-ethereum/libp2p/UPnP.h"
    )
endif()

