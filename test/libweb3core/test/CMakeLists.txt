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

# Disable Boost auto-linking, where boost libraries are automatically
# added to the link step for platforms which support that feature, which
# in our case appears only to be for Windows.   Presumably this is
# implemented using #pragma comment(lib ...)
#
# See https://support.microsoft.com/en-us/kb/153901.
#
# We don't want this automatic behavior, because it can add libraries
# to the link step which we don't actually need or want, depending on
# how cleanly #include dependencies have been managed, sometimes within
# header files which we don't even author.  It is much better for us
# just to manage these dependencies explicitly ourselves.
#
# See http://www.boost.org/doc/libs/1_40_0/more/getting_started/windows.html#auto-linking
#
add_definitions(-DBOOST_ALL_NO_LIB)

target_link_libraries(${EXECUTABLE} ${Boost_DATE_TIME_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${Boost_REGEX_LIBRARIES})
target_link_libraries(${EXECUTABLE} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
