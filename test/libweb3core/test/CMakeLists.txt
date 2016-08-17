include_directories(..)

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
file(GLOB HEADERS "*.h")

add_executable(testweb3core ${SRC_LIST} ${HEADERS})

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

include_directories(../../..)
include_directories(../../../utils)

find_package(Boost QUIET REQUIRED COMPONENTS chrono date_time regex unit_test_framework)
find_package(Dev)

target_link_libraries(testweb3core ${Boost_REGEX_LIBRARIES})
target_link_libraries(testweb3core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
if (DEFINED MSVC)
	target_link_libraries(testweb3core ${Boost_CHRONO_LIBRARIES})
	target_link_libraries(testweb3core ${Boost_DATE_TIME_LIBRARIES})
endif()
target_link_libraries(testweb3core ${Dev_DEVCORE_LIBRARIES})
target_link_libraries(testweb3core ${Dev_DEVCRYPTO_LIBRARIES})
target_link_libraries(testweb3core ${Dev_P2P_LIBRARIES})
if (UNIX AND NOT APPLE)
	target_link_libraries(testweb3core pthread)
endif()
