
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

if(EVMJIT OR HERA)
    list(APPEND SOURCES
        EVMC.cpp
        SmartVM.cpp
    )
endif()

file(GLOB HEADERS "*.h")

add_library(evm ${SOURCES} ${HEADERS})

target_link_libraries(evm PUBLIC ethcore devcore PRIVATE jsoncpp_lib_static Boost::program_options)
target_include_directories(evm PUBLIC ${CMAKE_SOURCE_DIR}/evmjit/include)
target_include_directories(evm PUBLIC ${CMAKE_SOURCE_DIR}/hera/src)

if (EVMJIT)
	target_link_libraries(evm PRIVATE evmjit)
	target_compile_definitions(evm PRIVATE ETH_EVMJIT)
endif()

if (HERA)
	target_link_libraries(evm PRIVATE hera)
    target_compile_definitions(evm PRIVATE ETH_HERA)
endif()
