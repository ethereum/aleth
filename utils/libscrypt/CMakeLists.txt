set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
file(GLOB HEADERS "*.h")

add_library(scrypt ${SRC_LIST} ${HEADERS})

install(TARGETS scrypt RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/scrypt)

