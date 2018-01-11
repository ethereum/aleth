
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

target_link_libraries(evm PUBLIC ethcore devcore PRIVATE jsoncpp_lib_static Boost::program_options)
target_include_directories(evm PUBLIC ${CMAKE_SOURCE_DIR}/evmjit/include)

if (EVMJIT)
	target_link_libraries(evm PRIVATE evmjit)
	target_compile_definitions(evm PRIVATE ETH_EVMJIT)
endif()
