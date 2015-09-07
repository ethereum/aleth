aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethminer)

file(GLOB HEADERS "*.h")

add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED JsonRpc::Client Dev::devcore Eth::ethcore)

jsonrcpstub_client_create(minerfarm.json FarmClient FarmClient)
jsonrcpstub_client_create(minerphonehome.json PhoneHome PhoneHome)

if (APPLE)
	install(TARGETS ${EXECUTABLE} DESTINATION bin)
else()
	eth_install_executable(${EXECUTABLE})
endif()
