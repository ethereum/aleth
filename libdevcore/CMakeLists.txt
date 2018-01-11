file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_library(devcore ${sources} ${headers})

add_dependencies(devcore BuildInfo.h)

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils)

target_link_libraries(devcore Boost::filesystem Boost::system Boost::thread Threads::Threads)

if (ROCKSDB)
    find_package(RocksDB)
    target_include_directories(devcore SYSTEM PUBLIC ${ROCKSDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${ROCKSDB_LIBRARIES})
else()
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore ${LEVELDB_LIBRARIES})
endif()
