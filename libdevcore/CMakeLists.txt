
add_library(
    devcore
    Address.cpp
    Address.h
    Base64.cpp
    Base64.h
    Common.cpp
    Common.h
    CommonData.cpp
    CommonData.h
    CommonIO.cpp
    CommonIO.h
    CommonJS.cpp
    CommonJS.h
    concurrent_queue.h
    db.h
    DBFactory.cpp
    DBFactory.h
    dbfwd.h
    Exceptions.h
    FileSystem.cpp
    FileSystem.h
    FixedHash.cpp
    FixedHash.h
    Guards.cpp
    Guards.h
    JsonUtils.cpp
    JsonUtils.h
    LevelDB.cpp
    LevelDB.h
    Log.cpp
    Log.h
    LoggingProgramOptions.cpp
    LoggingProgramOptions.h
    MemoryDB.cpp
    MemoryDB.h
    OverlayDB.cpp
    OverlayDB.h
    RLP.cpp
    RLP.h
    SHA3.cpp
    SHA3.h
    StateCacheDB.cpp
    StateCacheDB.h
    Terminal.h
    TransientDirectory.cpp
    TransientDirectory.h
    TrieCommon.cpp
    TrieCommon.h
    TrieDB.h
    TrieHash.cpp
    TrieHash.h
    UndefMacros.h
    vector_ref.h
    Worker.cpp
    Worker.h
)

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils)

target_link_libraries(devcore PUBLIC aleth-buildinfo Boost::filesystem Boost::system Boost::log Boost::thread Threads::Threads PRIVATE ethash::ethash)

hunter_add_package(leveldb)
find_package(leveldb CONFIG REQUIRED)
target_link_libraries(devcore PRIVATE leveldb::leveldb)

if(ROCKSDB)
    hunter_add_package(rocksdb)
    find_package(RocksDB CONFIG REQUIRED)
    target_sources(devcore PRIVATE RocksDB.cpp RocksDB.h)
    target_link_libraries(devcore PRIVATE RocksDB::rocksdb)
    target_compile_definitions(devcore PRIVATE ALETH_ROCKSDB)
endif()