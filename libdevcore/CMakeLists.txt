file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcore ${SRC_LIST} ${HEADERS})

add_dependencies(devcore BuildInfo.h)

target_include_directories(devcore PRIVATE ..)
# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(devcore Boost::filesystem Boost::system Boost::random Boost::thread)

if (ROCKSDB)
    find_package(RocksDB)
    target_include_directories(devcore SYSTEM PUBLIC ${ROCKSDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${ROCKSDB_LIBRARIES})
else()
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${LEVELDB_LIBRARIES})
endif()
