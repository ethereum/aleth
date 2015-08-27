# Find Web3
#
# Find the ethereum includes and library
#
# This module defines
#  Web3_XXX_LIBRARIES, the libraries needed to use ethereum.
#  TODO: Web3_INCLUDE_DIRS

set(LIBS whisper;webthree;web3jsonrpc)

set(WEB3_INCLUDE_DIRS ${WEB3_INCLUDE_DIR})

# if the project is a subset of webthree 
# use same pattern for variables as Boost uses
if (DEFINED webthree_VERSION)

	foreach (l ${LIBS}) 
		string(TOUPPER ${l} L)
		set ("Web3_${L}_LIBRARIES" ${l})
	endforeach()

else()

	foreach (l ${LIBS})
		string(TOUPPER ${l} L)
		find_library(Web3_${L}_LIBRARIES
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Release" 
			NO_DEFAULT_PATH
		)
	endforeach()

	# TODO: iterate over "lib${l}/Debug libraries if DEFINED MSVC
endif()

