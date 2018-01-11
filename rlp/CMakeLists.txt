add_executable(rlp main.cpp)
target_include_directories(rlp PRIVATE "${UTILS_INCLUDE_DIR}")
target_link_libraries(rlp PRIVATE devcrypto devcore Boost::program_options)
