# Find ethereum
#
# Find the ethereum includes and library
#
# This module defines
#  ETH_CORE_LIBRARIES, the libraries needed to use ethereum.
#  ETH_FOUND, If false, do not try to use ethereum.
#  TODO: ETH_INCLUDE_DIRS

set(CORE_LIBS ethereum;evm;ethcore;lll;p2p;evmasm;devcrypto;evmcore;devcore;ethash-cl;ethash;scrypt;natspec)
if (NOT MSVC)
	list(APPEND CORE_LIBS "secp256k1")
endif()

if (JSCONSOLE)
	list(APPEND CORE_LIBS jsengine jsconsole)
endif()

set(ALL_LIBS ${CORE_LIBS};evmjit;solidity;secp256k1;testutils)

set(ETH_INCLUDE_DIRS ${ETH_INCLUDE_DIR})

if (DEFINED ethereum_VERSION)
	set(ETH_CORE_LIBRARIES ${CORE_LIBS})
else()
	set(ETH_CORE_LIBRARIES ${ETH_LIBRARY})
	foreach (l ${ALL_LIBS})
		string(TOUPPER ${l} L)
		find_library(ETH_${L}_LIBRARY
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Release"	
			NO_DEFAULT_PATH
		)
	endforeach()

	foreach (l ${CORE_LIBS})
		string(TOUPPER ${l} L)
		list(APPEND ETH_CORE_LIBRARIES ${ETH_${L}_LIBRARY})
	endforeach()
endif()

# handle the QUIETLY and REQUIRED arguments and set ETH_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ethereum DEFAULT_MSG
	ETH_CORE_LIBRARIES)
mark_as_advanced (ETH_CORE_LIBRARIES)
