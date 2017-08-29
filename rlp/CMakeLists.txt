add_executable(rlp main.cpp)
target_include_directories(rlp PRIVATE ../utils)
target_link_libraries(rlp PRIVATE devcrypto devcore)
