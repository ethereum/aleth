
include_directories(..)

# A custom command and target to turn the OpenCL kernel into a byte array header
# The normal build depends on it properly and if the kernel file is changed, then
# a rebuild of libethash-cl should be triggered
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h
	COMMAND ${CMAKE_COMMAND} ARGS
	-DBIN2H_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/ethash_cl_miner_kernel.cl"
	-DBIN2H_VARIABLE_NAME=ethash_cl_miner_kernel
	-DBIN2H_HEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h"
	-P "${CMAKE_CURRENT_SOURCE_DIR}/bin2h.cmake"
	COMMENT "Generating OpenCL Kernel Byte Array"
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ethash_cl_miner_kernel.cl
)
add_custom_target(clbin2h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h ${CMAKE_CURRENT_SOURCE_DIR}/ethash_cl_miner_kernel.cl)

aux_source_directory(. SRC_LIST)
file(GLOB OUR_HEADERS "*.h")
set(HEADERS ${OUR_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h)

# TODO: Should fix properly. Cmake >= 3.4.0 detects a self linking of ethash-cl library here.
cmake_policy(SET CMP0038 OLD)
add_library(ethash-cl ${SRC_LIST} ${HEADERS})
eth_use(ethash-cl REQUIRED OpenCL)
target_include_directories(ethash-cl PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

find_package(Eth)

target_link_libraries(ethash-cl ${Eth_ETHASH_LIBRARIES})

install(TARGETS ethash-cl RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
