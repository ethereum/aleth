aux_source_directory(. SRC_LIST)

macro (add_sources)
	foreach (_src ${ARGN})
		file (RELATIVE_PATH _relPath "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}")
		list (APPEND SRC "./${_relPath}/${_src}")
		set(SRCTEST ${SRCTEST} ${SRC} PARENT_SCOPE)
	endforeach()
endmacro()

add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)

set(SRC_LIST ${SRC_LIST} ${SRCTEST})
MESSAGE(${SRC_LIST})

set(EXECUTABLE testweb3core)

file(GLOB HEADERS "*.h")
add_executable(${EXECUTABLE} ${SRC_LIST} ${HEADERS})
eth_use(${EXECUTABLE} REQUIRED Dev::devcrypto Dev::devcore Dev::p2p)

target_link_libraries(${EXECUTABLE} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})
