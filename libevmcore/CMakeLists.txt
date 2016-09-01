aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_library(evmcore ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED)

target_include_directories(evmcore PRIVATE ..)
target_include_directories(evmcore SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

# We have a very undesirable LevelDB dependency here due to devcore's inline
# vector_ref operator ldb::Slice().  We are stuck with this physical dependency
# even when that method is not used.   We should move that to a helper method
# outside of that header file which can be included only exactly where it is
# needed, and to avoid the LevelDB details to leak out of devcore.

if (NOT EMSCRIPTEN)
    find_package(LevelDB)
    target_include_directories(evmcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
    target_link_libraries(evmcore ${LEVELDB_LIBRARIES})
endif()

install(TARGETS evmcore RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
