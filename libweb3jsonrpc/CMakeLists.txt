set(sources
    AccountHolder.cpp
    AccountHolder.h
    AdminEth.cpp
    AdminEth.h
    AdminEthFace.h
    AdminNet.cpp
    AdminNet.h
    AdminNetFace.h
    Debug.cpp
    Debug.h
    DebugFace.h
    Eth.cpp
    Eth.h
    EthFace.h
    IpcServer.h
    IpcServerBase.cpp
    IpcServerBase.h
    JsonHelper.cpp
    JsonHelper.h
    ModularServer.h
    Net.cpp
    Net.h
    NetFace.h
    Personal.cpp
    Personal.h
    PersonalFace.h
    SessionManager.cpp
    SessionManager.h
    Test.cpp
    Test.h
    TestFace.h
    Web3.cpp
    Web3.h
    Web3Face.h
    WhisperFace.h
)

if(WIN32)
    list(APPEND sources WinPipeServer.cpp WinPipeServer.h)
else()
    list(APPEND sources UnixSocketServer.cpp UnixSocketServer.h)
endif()

add_library(web3jsonrpc ${sources})

jsonrpcstub_create(
    web3jsonrpc eth.json
    dev::rpc::EthFace ${CMAKE_CURRENT_SOURCE_DIR} EthFace
    EthClient ${CMAKE_CURRENT_BINARY_DIR} EthClient
)

jsonrpcstub_create(web3jsonrpc debug.json
    dev::rpc::DebugFace ${CMAKE_CURRENT_SOURCE_DIR} DebugFace
    DebugClient ${CMAKE_CURRENT_BINARY_DIR} DebugClient
)

jsonrpcstub_create(web3jsonrpc net.json
    dev::rpc::NetFace ${CMAKE_CURRENT_SOURCE_DIR} NetFace
    NetClient ${CMAKE_CURRENT_BINARY_DIR} NetClient
)

jsonrpcstub_create(web3jsonrpc web3.json
    dev::rpc::Web3Face ${CMAKE_CURRENT_SOURCE_DIR} Web3Face
    Web3Client ${CMAKE_CURRENT_BINARY_DIR} Web3Client
)

jsonrpcstub_create(web3jsonrpc admin_net.json
    dev::rpc::AdminNetFace ${CMAKE_CURRENT_SOURCE_DIR} AdminNetFace
    AdminNetClient ${CMAKE_CURRENT_BINARY_DIR} AdminNetClient
)

jsonrpcstub_create(web3jsonrpc admin_eth.json
    dev::rpc::AdminEthFace ${CMAKE_CURRENT_SOURCE_DIR} AdminEthFace
    AdminEthClient ${CMAKE_CURRENT_BINARY_DIR} AdminEthClient
)

jsonrpcstub_create(web3jsonrpc personal.json
    dev::rpc::PersonalFace ${CMAKE_CURRENT_SOURCE_DIR} PersonalFace
    PersonalClient ${CMAKE_CURRENT_BINARY_DIR} PersonalClient
)

jsonrpcstub_create(web3jsonrpc test.json
    dev::rpc::TestFace ${CMAKE_CURRENT_SOURCE_DIR} TestFace
    TestClient ${CMAKE_CURRENT_BINARY_DIR} TestClient
)

target_link_libraries(web3jsonrpc ethashseal webthree libjson-rpc-cpp::server)
