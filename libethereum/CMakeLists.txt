file(GLOB sources "*.cpp" "*.h")

add_library(ethereum ${sources})

target_include_directories(ethereum PRIVATE "${UTILS_INCLUDE_DIR}")
target_link_libraries(ethereum PUBLIC evm ethcore p2p devcrypto devcore PRIVATE jsoncpp_lib_static Snappy::snappy Boost::fiber Boost::context)
