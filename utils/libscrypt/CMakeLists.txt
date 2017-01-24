aux_source_directory(. SRC_LIST)
file(GLOB HEADERS "*.h")

add_library(scrypt ${SRC_LIST} ${HEADERS})

