
# Don't re-use SRC_LIST, because it has a special meaning within the test folder,
# with some dynamic aggregation of unit-testing going on.
aux_source_directory(. LOCAL_SRC_LIST)
include_directories(../..)

file(GLOB HEADERS "*.h")

add_library(testutils ${LOCAL_SRC_LIST} ${HEADERS})

find_package(Eth)

target_link_libraries(testutils ${Eth_ETHASHSEAL_LIBRARIES})
