file(GLOB sources "*.cpp" "*.h")

add_library(ethashseal ${sources})
target_link_libraries(ethashseal PUBLIC ethereum ethcore ethash)
