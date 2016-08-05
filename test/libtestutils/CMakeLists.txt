
# Don't re-use SRC_LIST, because it has a special meaning within the test folder,
# with some dynamic aggregation of unit-testing going on.
aux_source_directory(. LOCAL_SRC_LIST)

set(EXECUTABLE testutils)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${LOCAL_SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Eth::ethereum Eth::ethashseal)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )

