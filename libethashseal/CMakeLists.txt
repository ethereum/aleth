file(GLOB sources "*.cpp" "*.h")

add_library(ethashseal ${sources})
target_include_directories(ethashseal PRIVATE ../utils)
target_link_libraries(ethashseal PUBLIC ethereum ethcore ethash)
