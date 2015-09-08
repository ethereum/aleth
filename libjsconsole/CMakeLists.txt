aux_source_directory(. SRC_LIST)

set(EXECUTABLE jsconsole)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::jsengine Dev::devcore JsonRpc::Server JsonRpc::Client)
eth_use(${EXECUTABLE} OPTIONAL Readline)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )
