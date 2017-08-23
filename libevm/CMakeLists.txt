
set(SOURCES
	ExtVMFace.cpp
	Instruction.cpp
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

target_link_libraries(evm PRIVATE ethcore jsoncpp_lib_static)
target_include_directories(evm PUBLIC ../evmjit/include PRIVATE ..)

if (EVMJIT)
	target_link_libraries(evm PRIVATE evmjit)
endif()
