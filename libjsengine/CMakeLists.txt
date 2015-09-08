aux_source_directory(. SRC_LIST)

set(EXECUTABLE jsengine)

file(GLOB HEADERS "*.h")

eth_add_resources("${CMAKE_CURRENT_SOURCE_DIR}/JSResources.cmake" "JSRES" "${CMAKE_SOURCE_DIR}")
add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS} ${JSRES})

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
# macos brew version of v8 needs to be compiled with libstdc++
# it also needs to be dynamic library
# xcode needs libstdc++ to be explicitly set as it's attribute
if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
	set_property(TARGET ${EXECUTABLE} PROPERTY XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++")
endif()

eth_use(${EXECUTABLE} REQUIRED V8)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )
