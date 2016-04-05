aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethashseal)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
eth_use(${EXECUTABLE} REQUIRED Eth::ethereum Eth::ethcore Eth::ethash Dev::devcrypto Dev::buildinfo)
eth_use(${EXECUTABLE} OPTIONAL Eth::ethash-cl Cpuid)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

