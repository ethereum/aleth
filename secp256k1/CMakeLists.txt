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

if (APPLE OR UNIX)

	if (ETH_STATIC)
		add_library(${EXECUTABLE} STATIC ${EXECUTABLE}.c field_5x52_impl.h)
	else()
		add_library(${EXECUTABLE} SHARED ${EXECUTABLE}.c field_5x52_impl.h)
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -DHAVE___INT128 -DUSE_SCALAR_4X64 -DUSE_SCALAR_INV_NUM -DUSE_ASM_X86_64 -DUSE_FIELD_5X52 -DUSE_FIELD_INV_NUM -DUSE_NUM_GMP -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings")
	target_link_libraries(${EXECUTABLE} ${GMP_LIBRARIES})

elseif (CMAKE_COMPILER_IS_MINGW)

	include_directories(${Boost_INCLUDE_DIRS})
	if (ETH_STATIC)
		add_library(${EXECUTABLE} STATIC ${EXECUTABLE}.c field_5x52_impl.h)
	else()
		add_library(${EXECUTABLE} SHARED ${EXECUTABLE}.c field_5x52_impl.h)
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -DHAVE___INT128 -DUSE_SCALAR_4X64 -DUSE_SCALAR_INV_NUM -DUSE_ASM_X86_64 -DUSE_FIELD_5X52 -DUSE_FIELD_INV_NUM -DUSE_NUM_GMP -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings")
	target_link_libraries(${EXECUTABLE} ${GMP_LIBRARIES})
  
else()

	include_directories(${Boost_INCLUDE_DIRS})
	if (ETH_STATIC)
		add_library(${EXECUTABLE} STATIC ${EXECUTABLE}.c)
	else()
		add_library(${EXECUTABLE} SHARED ${EXECUTABLE}.c)
	endif()
	# /TP - compile project as cpp project
	set_target_properties(${EXECUTABLE} PROPERTIES COMPILE_FLAGS "/TP /wd4244")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SCALAR_4X64 -DUSE_SCALAR_INV_NUM -DUSE_ASM_X86_64 -DUSE_NUM_BOOST -DUSE_FIELD_10X26 -DUSE_FIELD_INV_BUILTIN")
	
endif()

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )
