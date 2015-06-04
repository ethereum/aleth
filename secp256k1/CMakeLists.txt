cmake_policy(SET CMP0015 NEW)
# this policy was introduced in cmake 3.0
# remove if, once 3.0 will be used on unix
if (${CMAKE_MAJOR_VERSION} GREATER 2)
	# old policy do not use MACOSX_RPATH
	cmake_policy(SET CMP0042 OLD)
endif()
set(CMAKE_AUTOMOC OFF)

set(EXECUTABLE secp256k1)
file(GLOB HEADERS "*.h") 

if (DEFINED WIN32)
	include_directories(${Boost_INCLUDE_DIRS})
	add_library(${EXECUTABLE} ${EXECUTABLE}.c field_10x26_impl.h)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DUSE_FIELD_10X26 -DUSE_SCALAR_4X64 -DUSE_SCALAR_INV_NUM -DUSE_ASM_X86_64 -DUSE_NUM_BOOST -DUSE_FIELD_INV_BUILTIN -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings")
	# /TP - compile project as cpp project
	set_target_properties(${EXECUTABLE} PROPERTIES COMPILE_FLAGS "/TP /wd4244")
else()
	add_library(${EXECUTABLE} ${EXECUTABLE}.c field_5x52_impl.h)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DUSE_FIELD_5X52 -DHAVE___INT128 -DUSE_SCALAR_4X64 -DUSE_FIELD_INV_NUM -DUSE_SCALAR_INV_NUM -DUSE_ASM_X86_64 -DUSE_NUM_GMP -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings")
endif()

target_link_libraries(${EXECUTABLE} ${GMP_LIBRARIES})

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )
