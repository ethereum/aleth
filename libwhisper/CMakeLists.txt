set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
include_directories(..)
include_directories(../utils)

file(GLOB HEADERS "*.h")

add_library(whisper ${SRC_LIST} ${HEADERS})

find_package(Boost QUIET REQUIRED COMPONENTS regex)
find_package(Dev)

target_link_libraries(whisper ${Boost_REGEX_LIBRARIES})
target_link_libraries(whisper ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(whisper ${Dev_DEVCRYPTO_LIBRARIES})
target_link_libraries(whisper ${Dev_P2P_LIBRARIES})

install(TARGETS whisper RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
