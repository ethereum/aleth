
set(sources
    ExtVMFace.cpp ExtVMFace.h
    Instruction.cpp Instruction.h
    VMFace.h
    VMConfig.h
    VM.cpp VM.h
    VMCalls.cpp
    VMOpt.cpp
    VMSIMD.cpp
    VMValidate.cpp
    VMFactory.cpp VMFactory.h
)

if(EVMJIT OR HERA)
    list(APPEND sources
        EVMC.cpp EVMC.h
        HeraPlusVM.cpp HeraPlusVM.h
    )
endif()

add_library(evm ${sources})

target_link_libraries(evm PUBLIC ethcore devcore PRIVATE jsoncpp_lib_static Boost::program_options)
target_include_directories(evm PUBLIC ${CMAKE_SOURCE_DIR}/evmjit/include)
target_include_directories(evm PUBLIC ${CMAKE_SOURCE_DIR}/hera/src)

if(EVMJIT)
    target_link_libraries(evm PRIVATE evmjit)
    target_compile_definitions(evm PRIVATE ETH_EVMJIT)
endif()

if(HERA)
    target_link_libraries(evm PRIVATE hera)
    target_compile_definitions(evm PRIVATE ETH_HERA)
endif()
