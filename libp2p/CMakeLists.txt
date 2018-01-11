file(GLOB sources "*.cpp" "*.h")

add_library(p2p ${sources})
target_link_libraries(p2p PUBLIC devcrypto devcore)
target_include_directories(p2p SYSTEM PRIVATE ${CRYPTOPP_INCLUDE_DIR})

if(MINIUPNPC)
    target_link_libraries(p2p PRIVATE "${MINIUPNPC_LIBRARY}")
    target_include_directories(p2p SYSTEM PRIVATE "${MINIUPNPC_INCLUDE_DIR}")
    target_compile_definitions(p2p PRIVATE ETH_MINIUPNPC)
endif()