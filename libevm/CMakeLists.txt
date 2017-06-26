
set(SOURCES
	ExtVMFace.cpp
	VM.cpp
	VMOpt.cpp
	VMCalls.cpp
	VMSIMD.cpp
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

# TODO: We have horrible cilcular dependency: evm depends also on ethereum
#       library. Here we only explicitly add jsoncpp to fix includes.
target_link_libraries(evm PRIVATE evmcore ethcore JsonCpp)
target_include_directories(evm PRIVATE ..)

if (EVMJIT)
	target_link_libraries(evm PRIVATE evmjit)
endif()
