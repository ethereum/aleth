file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcore ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED COMPONENTS thread filesystem random system)

target_include_directories(devcore PRIVATE ..)
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(devcore ${Boost_FILESYSTEM_LIBRARIES})
target_link_libraries(devcore ${Boost_RANDOM_LIBRARIES})
target_link_libraries(devcore ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(devcore ${Boost_THREAD_LIBRARIES})

if (NOT EMSCRIPTEN)
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${LEVELDB_LIBRARIES})
endif()

install(TARGETS devcore RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
