
set(SOURCES
	ExtVMFace.cpp
	VM.cpp
	VMFactory.cpp
)

if (EVMJIT)
	list(APPEND SOURCES
		JitEnv.cpp
		JitVM.cpp
		SmartVM.cpp
	)
endif()

# we may not use it in libevm, but one of our dependecies is including boost in header file
# and windows is failing to build without that
if (EVMJIT)
	include_directories(../evmjit/include)
endif()

set(EXECUTABLE evm)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SOURCES} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Eth::ethcore Dev::devcrypto Eth::evmcore Dev::devcore)
eth_use(${EXECUTABLE} OPTIONAL Eth::evmjit)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/libethereum/libevm )
