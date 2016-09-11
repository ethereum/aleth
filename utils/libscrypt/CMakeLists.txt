set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)
file(GLOB HEADERS "*.h")

add_library(scrypt ${SRC_LIST} ${HEADERS})

eth_install_library(scrypt)

