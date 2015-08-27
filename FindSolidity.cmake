# Find Solidity
#
# Find the solidity includes and library
#
# This module defines
#  SOLIDITY_LIBRARIES, the libraries needed to use solidity.
#  TODO: SOLIDITY_INCLUDE_DIRS

set(l solidity)

# if the project is a subset of main cpp-solidity project
# use same pattern for variables as Boost uses
if (DEFINED solidity_VERSION)

	string(TOUPPER ${l} L)
	set ("SOLIDITY_LIBRARIES" ${l})

else()

	string(TOUPPER ${l} L)
	find_library(SOLIDITY_LIBRARIES
		NAMES ${l}
		PATHS ${CMAKE_LIBRARY_PATH}
		PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Release" 
		NO_DEFAULT_PATH
	)

	# TODO: look in over "lib${l}/Debug libraries if DEFINED MSVC
endif()

