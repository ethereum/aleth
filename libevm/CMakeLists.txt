
set(sources
    EVMC.cpp EVMC.h
    ExtVMFace.cpp ExtVMFace.h
    Instruction.cpp Instruction.h
    interpreter.h
    LegacyVM.cpp LegacyVM.h
    LegacyVMCalls.cpp
    LegacyVMOpt.cpp
    VMFace.h
    VMConfig.h
    VM.cpp VM.h
    VMCalls.cpp
    VMOpt.cpp
    VMSIMD.cpp
    VMValidate.cpp
    VMFactory.cpp VMFactory.h
)

add_library(evm ${sources})

target_link_libraries(evm PUBLIC ethcore devcore evmc PRIVATE jsoncpp_lib_static Boost::program_options)

if(EVM_OPTIMIZE)
    target_compile_definitions(evm PRIVATE EVM_OPTIMIZE)
endif()

if(EVMJIT)
    target_link_libraries(evm PRIVATE evmjit)
    target_compile_definitions(evm PRIVATE ETH_EVMJIT)
endif()

if(HERA)
    target_link_libraries(evm PRIVATE hera)
    target_compile_definitions(evm PRIVATE ETH_HERA)
endif()
