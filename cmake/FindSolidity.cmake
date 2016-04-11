# Find Solidity
#
# Find the solidity includes and library
#
# This module defines
#  SOLIDITY_LIBRARIES, the libraries needed to use solidity.
#  SOLIDITY_INCLUDE_DIRS

include(EthUtils)
set(l solidity)

set(SOLIDITY_INCLUDE_DIRS "${SOL_DIR}")

# if the project is a subset of main cpp-ethereum project
# use same pattern for variables as Boost uses
if ((DEFINED solidity_VERSION) OR (DEFINED cpp-ethereum_VERSION))

	string(TOUPPER ${l} L)
	set ("SOLIDITY_LIBRARIES" ${l})

else()

	string(TOUPPER ${l} L)
	find_library(SOLIDITY_LIBRARY
		NAMES ${l}
		PATHS ${CMAKE_LIBRARY_PATH}
		PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Debug" "lib${l}/Release"
		NO_DEFAULT_PATH
	)

	set(SOLIDITY_LIBRARIES ${SOLIDITY_LIBRARY})

	if (DEFINED MSVC)
		find_library(SOLIDITY_LIBRARY_DEBUG
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}/Debug" 
			NO_DEFAULT_PATH
		)
		eth_check_library_link(SOLIDITY_LIBRARY)
	endif()

endif()
