add_executable(aleth-vm main.cpp)

target_link_libraries(aleth-vm PRIVATE ethereum evm ethashseal devcore Boost::program_options)

target_include_directories(aleth-vm PRIVATE ../utils)

install(TARGETS aleth-vm EXPORT alethTargets DESTINATION bin)
