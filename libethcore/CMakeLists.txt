file(GLOB sources "*.cpp" "*.h")

add_library(ethcore ${sources})

target_include_directories(ethcore PRIVATE ../utils)
target_link_libraries(ethcore PUBLIC devcrypto devcore)
