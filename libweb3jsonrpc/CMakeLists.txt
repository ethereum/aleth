aux_source_directory(. SRC_LIST)

set(EXECUTABLE web3jsonrpc)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::whisper Web3::webthree JsonRpc::Server)

jsonrpcstub_create(spec.json 
	AbstractWebThreeStubServer ${CMAKE_CURRENT_SOURCE_DIR} AbstractWebThreeStubServer
	WebThreeStubClient ${CMAKE_CURRENT_BINARY_DIR} WebThreeStubClient
)

jsonrpcstub_create(db.json
	dev::rpc::DBFace ${CMAKE_CURRENT_SOURCE_DIR} DBFace
	DBClient ${CMAKE_CURRENT_BINARY_DIR} DBClient
)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

add_custom_target(aux_json SOURCES "spec.json")
