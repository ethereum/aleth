set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
include_directories(..)

file(GLOB HEADERS "*.h")

add_library(devcore ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED COMPONENTS thread filesystem random system)
find_package(LevelDB)

target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
if (NOT EMSCRIPTEN)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
endif()

target_link_libraries(devcore ${Boost_FILESYSTEM_LIBRARIES})
target_link_libraries(devcore ${Boost_RANDOM_LIBRARIES})
target_link_libraries(devcore ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(devcore ${Boost_THREAD_LIBRARIES})
if (NOT EMSCRIPTEN)
    target_link_libraries(devcore ${LEVELDB_LIBRARIES})
endif()

install(TARGETS devcore RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
