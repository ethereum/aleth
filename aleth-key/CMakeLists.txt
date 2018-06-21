add_executable(aleth-key KeyAux.h main.cpp)
target_link_libraries(aleth-key PRIVATE ethcore devcore Boost::program_options)
install(TARGETS aleth-key EXPORT alethTargets DESTINATION bin)
