aux_source_directory(. SRC_LIST)

set(EXECUTABLE web3jsonrpc)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::whisper Web3::webthree JsonRpc::Server)

jsonrpcstub_create(eth.json 
	dev::rpc::EthFace ${CMAKE_CURRENT_SOURCE_DIR} EthFace 
	EthClient ${CMAKE_CURRENT_BINARY_DIR} EthClient
)

jsonrpcstub_create(db.json
	dev::rpc::DBFace ${CMAKE_CURRENT_SOURCE_DIR} DBFace
	DBClient ${CMAKE_CURRENT_BINARY_DIR} DBClient
)

jsonrpcstub_create(whisper.json
	dev::rpc::WhisperFace ${CMAKE_CURRENT_SOURCE_DIR} WhisperFace
	WhisperClient ${CMAKE_CURRENT_BINARY_DIR} WhisperClient
)

jsonrpcstub_create(net.json
	dev::rpc::NetFace ${CMAKE_CURRENT_SOURCE_DIR} NetFace
	NetClient ${CMAKE_CURRENT_BINARY_DIR} NetClient
)

jsonrpcstub_create(bzz.json
	dev::rpc::BzzFace ${CMAKE_CURRENT_SOURCE_DIR} BzzFace
	BzzClient ${CMAKE_CURRENT_BINARY_DIR} BzzClient
)

jsonrpcstub_create(web3.json
	dev::rpc::Web3Face ${CMAKE_CURRENT_SOURCE_DIR} Web3Face
	Web3Client ${CMAKE_CURRENT_BINARY_DIR} Web3Client
)

jsonrpcstub_create(admin_net.json
	dev::rpc::AdminNetFace ${CMAKE_CURRENT_SOURCE_DIR} AdminNetFace
	AdminNetClient ${CMAKE_CURRENT_BINARY_DIR} AdminNetClient
)

jsonrpcstub_create(admin_eth.json
	dev::rpc::AdminEthFace ${CMAKE_CURRENT_SOURCE_DIR} AdminEthFace
	AdminEthClient ${CMAKE_CURRENT_BINARY_DIR} AdminEthClient
)

jsonrpcstub_create(admin_utils.json
	dev::rpc::AdminUtilsFace ${CMAKE_CURRENT_SOURCE_DIR} AdminUtilsFace
	AdminUtilsClient ${CMAKE_CURRENT_BINARY_DIR} AdminUtilsClient
)

jsonrpcstub_create(personal.json
	dev::rpc::PersonalFace ${CMAKE_CURRENT_SOURCE_DIR} PersonalFace
	PersonalClient ${CMAKE_CURRENT_BINARY_DIR} PersonalClient
)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

