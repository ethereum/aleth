file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(ethereum ${SRC_LIST} ${HEADERS})

target_link_libraries(ethereum ${Boost_REGEX_LIBRARIES} JsonCpp)

find_package(Dev)
find_package(Eth)

target_include_directories(ethereum PRIVATE ..)
target_include_directories(ethereum PRIVATE ../utils)
target_link_libraries(ethereum ${Eth_ETHCORE_LIBRARIES})
target_link_libraries(ethereum ${Eth_EVM_LIBRARIES})

if (NOT EMSCRIPTEN)
	target_link_libraries(ethereum ${Dev_P2P_LIBRARIES})
	target_link_libraries(ethereum ${Dev_DEVCRYPTO_LIBRARIES})
endif()

if (EVMJIT)
	target_link_libraries(ethereum ${Eth_EVMJIT_LIBRARIES})
endif()
