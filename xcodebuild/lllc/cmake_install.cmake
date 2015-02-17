# Install script for directory: /Users/alex/dev/cpp-ethereum/lllc

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
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/lllc/Debug/lllc")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Debug/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/Debug/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/Debug/liblll.dylib" "liblll.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/lllc/Release/lllc")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/Release/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/Release/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/Release/liblll.dylib" "liblll.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/lllc/MinSizeRel/lllc")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/MinSizeRel/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/MinSizeRel/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/MinSizeRel/liblll.dylib" "liblll.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      endif()
    endif()
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/alex/dev/cpp-ethereum/xcodebuild/lllc/RelWithDebInfo/lllc")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND "/usr/bin/install_name_tool"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/RelWithDebInfo/libdevcore.dylib" "libdevcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/RelWithDebInfo/libevmcore.dylib" "libevmcore.dylib"
        -change "/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/RelWithDebInfo/liblll.dylib" "liblll.dylib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      execute_process(COMMAND /usr/bin/install_name_tool
        -delete_rpath "/usr/local/lib"
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lllc")
      endif()
    endif()
  endif()
endif()

