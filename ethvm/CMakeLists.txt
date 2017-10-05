add_executable(ethvm main.cpp)

target_link_libraries(ethvm PRIVATE ethereum evm ethashseal devcore Boost::program_options)

install(TARGETS ethvm DESTINATION bin OPTIONAL)
