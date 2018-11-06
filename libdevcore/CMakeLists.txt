file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_library(devcore ${sources} ${headers})

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils)

target_link_libraries(devcore PUBLIC aleth-buildinfo Boost::filesystem Boost::system Boost::log Boost::thread Threads::Threads PRIVATE ethash::ethash)

if (MSVC)
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(devcore PRIVATE ${LEVELDB_LIBRARIES})
else()
    hunter_add_package(leveldb)
    find_package(leveldb CONFIG REQUIRED)
    target_link_libraries(devcore PRIVATE leveldb::leveldb)
endif()

hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)
target_link_libraries(devcore PRIVATE RocksDB::rocksdb)
