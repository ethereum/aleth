set(
    sources
    main.cpp
)

add_executable(aleth-bootnode ${sources})
target_link_libraries(
    aleth-bootnode
    PRIVATE p2p devcore Boost::program_options
)

install(TARGETS aleth-bootnode DESTINATION bin)
