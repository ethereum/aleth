aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(evmcore ${SRC_LIST} ${HEADERS})

target_include_directories(evmcore PRIVATE ..)
target_link_libraries(evmcore devcore Boost::Filesystem)
