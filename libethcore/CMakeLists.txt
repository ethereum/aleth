aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethcore)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Dev::devcrypto Eth::ethash Dev::buildinfo)
eth_use(${EXECUTABLE} OPTIONAL Eth::ethash-cl Cpuid)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

