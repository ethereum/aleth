aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

add_library(evmcore ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED)
target_include_directories(evmcore SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

install(TARGETS evmcore RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
