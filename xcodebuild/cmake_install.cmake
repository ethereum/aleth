# Install script for directory: /Users/alex/dev/cpp-ethereum

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcore/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libevmcore/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/liblll/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libserpent/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/sc/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libsolidity/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/lllc/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/solc/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libweb3jsonrpc/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/secp256k1/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libp2p/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libdevcrypto/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libwhisper/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libethcore/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libevm/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libethereum/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libwebthree/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/test/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/eth/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/neth/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libnatspec/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/libjsqrc/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/alethzero/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/third/cmake_install.cmake")
  include("/Users/alex/dev/cpp-ethereum/xcodebuild/mix/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

file(WRITE "/Users/alex/dev/cpp-ethereum/xcodebuild/${CMAKE_INSTALL_MANIFEST}" "")
foreach(file ${CMAKE_INSTALL_MANIFEST_FILES})
  file(APPEND "/Users/alex/dev/cpp-ethereum/xcodebuild/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
endforeach()
