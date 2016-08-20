
set(SOURCES
	ExtVMFace.cpp
	VM.cpp
	VMNoInline.cpp
	VMFactory.cpp
)

if (EVMJIT)
	list(APPEND SOURCES
		JitVM.cpp
		SmartVM.cpp
	)
endif()

# We are deliberately NOT supporting BulkBuild for libevm, because
# some of Greg Colvin's EVM optimizations explicitly depend on the
# compilation unit boundaries which we deliberately put in place with
# the creation of VMNoInline.cpp.
#
# As-and-when we add the ability to control BulkBuild unit boundaries
# better we could add BulkBuild support - but with that compilation
# unit excluded from the BulkBuild unit.
#enable_bulkbuild(evm SOURCES)

file(GLOB HEADERS "*.h")

add_library(evm ${SOURCES} ${HEADERS})

find_package(Eth)

target_include_directories(evm PRIVATE ..)
target_link_libraries(evm ${Eth_ETHCORE_LIBRARIES})
target_link_libraries(evm ${Eth_EVMCORE_LIBRARIES})

if (EVMJIT)
	target_include_directories(evm PRIVATE ../evmjit/include)
	target_link_libraries(evm ${Eth_EVMJIT_LIBRARIES})
endif()

install(TARGETS evm RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
