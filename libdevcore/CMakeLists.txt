file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_library(devcore ${sources} ${headers})

add_dependencies(devcore BuildInfo.h)

# This "local" include path will be propagated to all other libs.
target_include_directories(devcore PUBLIC ..)

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
