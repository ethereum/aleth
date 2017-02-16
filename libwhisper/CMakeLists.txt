aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(whisper ${SRC_LIST} ${HEADERS})

find_package(Dev)

target_include_directories(whisper PRIVATE ..)
target_include_directories(whisper PRIVATE ../utils)
target_link_libraries(whisper ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(whisper ${Dev_DEVCRYPTO_LIBRARIES})
target_link_libraries(whisper ${Dev_P2P_LIBRARIES})
