function(eth_apply TARGET REQUIRED)
    find_package(Eth)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
    include_directories(${CPPETHEREUM_BUILD})
    target_link_libraries(${EXECUTABLE} ${Boost_THREAD_LIBRARIES})
    target_link_libraries(${EXECUTABLE} ${Boost_RANDOM_LIBRARIES})
    target_link_libraries(${EXECUTABLE} ${Boost_FILESYSTEM_LIBRARIES})
    target_link_libraries(${EXECUTABLE} ${Boost_SYSTEM_LIBRARIES})
    target_link_libraries(${EXECUTABLE} ${LEVELDB_LIBRARIES})	#TODO: use the correct database library according to cpp-ethereum
    target_link_libraries(${EXECUTABLE} ${CRYPTOPP_LIBRARIES})
    target_link_libraries(${TARGET} ${ETH_CORE_LIBRARIES})
    if (UNIX)
        target_link_libraries(${EXECUTABLE} pthread)
    endif()
    eth_install_executable(${EXECUTABLE} DLLS EVMJIT_DLLS OpenCL_DLLS)
endfunction()


