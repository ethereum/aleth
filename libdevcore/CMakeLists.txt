file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcore ${SRC_LIST} ${HEADERS})

target_include_directories(devcore PRIVATE ..)
target_include_directories(devcore SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})

add_dependencies(devcore BuildInfo.h)

if (NOT EMSCRIPTEN)
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore boost::filesystem boost::random boost::thread ${LEVELDB_LIBRARIES})
endif()
