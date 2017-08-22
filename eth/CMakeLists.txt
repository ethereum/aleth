set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.h
)

add_executable(eth ${sources})
target_link_libraries(
    eth
    PUBLIC ethereum ethashseal evm web3jsonrpc webthree devcore
    PRIVATE Threads::Threads
)

install(TARGETS eth DESTINATION bin)
