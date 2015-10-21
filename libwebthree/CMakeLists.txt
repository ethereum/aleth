set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

set(EXECUTABLE webthree)

file(GLOB HEADERS "*.h")
list(APPEND SRC_LIST libexecstream/exec-stream.cpp)
if (MSVC)
	list(APPEND SRC_LIST libexecstream/win/exec-stream-helpers.cpp libexecstream/win/exec-stream-impl.cpp)
else()
	list(APPEND SRC_LIST libexecstream/posix/exec-stream-helpers.cpp libexecstream/posix/exec-stream-impl.cpp)
endif()

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

eth_use(${EXECUTABLE} REQUIRED Web3::whisper Eth::ethereum)

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

