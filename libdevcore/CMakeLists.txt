file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcore ${SRC_LIST} ${HEADERS})

target_include_directories(devcore PRIVATE ..)
target_include_directories(devcore SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})

add_dependencies(devcore BuildInfo.h)

target_link_libraries(devcore Boost::Filesystem Boost::Random Boost::Thread)

if (ROCKSDB)
    find_package(RocksDB)
    target_include_directories(devcore SYSTEM PUBLIC ${ROCKSDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${ROCKSDB_LIBRARIES})
else()
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${LEVELDB_LIBRARIES})
endif()
