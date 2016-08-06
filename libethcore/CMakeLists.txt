aux_source_directory(. SRC_LIST)

set(EXECUTABLE ethcore)

file(GLOB HEADERS "*.h")

include_directories(BEFORE ..)
add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Dev::devcrypto Dev::buildinfo Dev::devcore)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )

