aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_subdirectory(genesis)
aux_source_directory(./genesis SRCS)
set(SRC_LIST ${SRC_LIST} ${SRCS})

add_library(ethashseal ${SRC_LIST} ${HEADERS})
eth_use(ethashseal OPTIONAL Eth::ethash-cl Cpuid)

find_package(Eth)

target_include_directories(ethashseal PRIVATE ..)
target_link_libraries(ethashseal ${Eth_ETHASH_LIBRARIES})
target_link_libraries(ethashseal ${Eth_ETHEREUM_LIBRARIES})

if (ETHASHCL)
    eth_use(ethashseal REQUIRED OpenCL)
    if (OpenCL_FOUND)
        target_include_directories(ethashseal SYSTEM PUBLIC ${OpenCL_INCLUDE_DIRS})
        target_link_libraries(ethashseal ${Eth_ETHASH-CL_LIBRARIES})
        target_compile_definitions(ethashseal PUBLIC ETH_ETHASHCL)
    endif()
endif()

install(TARGETS ethashseal RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
