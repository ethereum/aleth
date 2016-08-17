aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

eth_simple_add_executable(ethminer ${SRC_LIST} ${HEADERS})

eth_use(ethminer REQUIRED JsonRpc::Client)

jsonrpcstub_client_create(ethminer minerfarm.json FarmClient ${CMAKE_CURRENT_SOURCE_DIR} FarmClient)
jsonrpcstub_client_create(ethminer minerphonehome.json PhoneHome ${CMAKE_CURRENT_SOURCE_DIR} PhoneHome)

find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time)
find_package(Eth)

if (DEFINED MSVC)
	target_link_libraries(ethminer ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(ethminer ${Boost_DATE_TIME_LIBRARIES})
endif()
target_link_libraries(ethminer ${Eth_ETHASHSEAL_LIBRARIES})
if (UNIX AND NOT APPLE)
	target_link_libraries(ethminer pthread)
endif()

if (APPLE)
	install(TARGETS ethminer DESTINATION bin)
else()
	eth_install_executable(ethminer)
endif()
