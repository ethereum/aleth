aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(ethcore ${SRC_LIST} ${HEADERS})

find_package(Dev)

target_include_directories(ethcore PRIVATE ..)
target_include_directories(ethcore PRIVATE ../utils)
target_link_libraries(ethcore ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(ethcore ${Dev_DEVCRYPTO_LIBRARIES})
