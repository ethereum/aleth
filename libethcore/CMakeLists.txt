aux_source_directory(. SRC_LIST)
include_directories(..)
include_directories(../utils)

file(GLOB HEADERS "*.h")

add_library(ethcore ${SRC_LIST} ${HEADERS})

find_package(Dev)

target_link_libraries(ethcore ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(ethcore ${Dev_DEVCRYPTO_LIBRARIES})

install(TARGETS ethcore RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

