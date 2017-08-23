add_library(webthree WebThree.cpp WebThree.h)
target_link_libraries(webthree PUBLIC ethereum whisper p2p devcore)
