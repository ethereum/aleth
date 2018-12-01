set(
    sources
    main.cpp
)

add_executable(aleth-bootnode ${sources})
target_link_libraries(
    aleth-bootnode
    PRIVATE devcore Boost::program_options p2p
)

install(TARGETS aleth-bootnode DESTINATION bin)
