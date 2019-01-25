set(sources
    Ethash.cpp
    Ethash.h
    EthashCPUMiner.cpp
    EthashCPUMiner.h
    EthashProofOfWork.cpp
    EthashProofOfWork.h
    GenesisInfo.cpp
    GenesisInfo.h
)

add_library(ethashseal ${sources})
target_include_directories(ethashseal PRIVATE ../utils)
target_link_libraries(ethashseal PUBLIC ethereum ethcore)
