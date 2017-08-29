file(GLOB sources "*.cpp" "*.h")

add_library(whisper ${sources})
target_link_libraries(whisper PUBLIC p2p devcrypto devcore)
