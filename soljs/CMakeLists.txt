cmake_policy(SET CMP0015 NEW)

aux_source_directory(. SRC_LIST)

include_directories(..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_FUNCTIONS='[\"_compileString\"]'")

set(EXECUTABLE soljs)

add_executable(${EXECUTABLE} ${SRC_LIST})

target_link_libraries(${EXECUTABLE} solidity)
target_link_libraries(${EXECUTABLE} devcore)

