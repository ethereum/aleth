add_executable(ethkey KeyAux.h main.cpp)
target_link_libraries(ethkey PRIVATE ethcore devcore Boost::program_options)
