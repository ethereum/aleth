cmake_policy(SET CMP0015 NEW)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

set(EXECUTABLE lll)

file(GLOB HEADERS "*.h")
if(ETH_STATIC)
	add_library(${EXECUTABLE} STATIC ${SRC_LIST} ${HEADERS})
else()
	add_library(${EXECUTABLE} SHARED ${SRC_LIST} ${HEADERS})
endif()

include_directories(..)

target_link_libraries(${EXECUTABLE} evmface)
target_link_libraries(${EXECUTABLE} devcore)


if("${TARGET_PLATFORM}" STREQUAL "w64")
	target_link_libraries(${EXECUTABLE} boost_system-mt-s)
	target_link_libraries(${EXECUTABLE} boost_thread_win32-mt-s)
	target_link_libraries(${EXECUTABLE} iphlpapi)
	target_link_libraries(${EXECUTABLE} ws2_32)
	target_link_libraries(${EXECUTABLE} mswsock)
	target_link_libraries(${EXECUTABLE} shlwapi)
elseif (APPLE)
elseif (UNIX)
else ()
endif ()

install( TARGETS ${EXECUTABLE} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

