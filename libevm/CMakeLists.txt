
set(SOURCES
	ExtVMFace.cpp
	VM.cpp
	VMOpt.cpp
	VMCalls.cpp
	VMValidate.cpp
	VMFactory.cpp
)

if (EVMJIT)
	list(APPEND SOURCES
		JitVM.cpp
		SmartVM.cpp
	)
endif()

file(GLOB HEADERS "*.h")

add_library(evm ${SOURCES} ${HEADERS})

find_package(Eth)

target_link_libraries(evm ${Eth_ETHCORE_LIBRARIES})
target_link_libraries(evm ${Eth_EVMCORE_LIBRARIES})
target_include_directories(evm PRIVATE .. ${JSONCPP_INCLUDE_DIR})

if (EVMJIT)
	target_include_directories(evm PRIVATE ../evmjit/include)
	target_link_libraries(evm ${Eth_EVMJIT_LIBRARIES})
endif()
