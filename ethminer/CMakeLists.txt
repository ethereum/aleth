aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethminer)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED JsonRpc::Client Eth::ethashseal)

jsonrpcstub_client_create(minerfarm.json FarmClient ${CMAKE_CURRENT_SOURCE_DIR} FarmClient)
jsonrpcstub_client_create(minerphonehome.json PhoneHome ${CMAKE_CURRENT_SOURCE_DIR} PhoneHome)

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
