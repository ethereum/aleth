# Find Eth
#
# Find the ethereum includes and library
#
# This module defines
#  ETH_XXX_LIBRARIES, the libraries needed to use ethereum.
#  ETH_FOUND, If false, do not try to use ethereum.
#  TODO: ETH_INCLUDE_DIRS

set(LIBS ethereum;evm;ethcore;lll;p2p;evmasm;devcrypto;evmcore;devcore;ethash-cl;ethash;scrypt;natspec;jsengine;jsconsole;evmjit;evmjit-cpp;solidity;secp256k1;testutils)

set(ETH_INCLUDE_DIRS ${ETH_INCLUDE_DIR})

# if the project is a subset of main cpp-ethereum project
# use same pattern for variables as Boost uses
if (DEFINED ethereum_VERSION)

	foreach (l ${LIBS}) 
		string(TOUPPER ${l} L)
		set ("Eth_${L}_LIBRARIES" ${l})
	endforeach()

else()

	foreach (l ${LIBS})
		string(TOUPPER ${l} L)
		find_library(Eth_${L}_LIBRARIES
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Release" 
			# libevmjit is nested...
			"evmjit/libevmjit-cpp"
			NO_DEFAULT_PATH
		)
	endforeach()

	# TODO: iterate over "lib${l}/Debug libraries if DEFINED MSVC
endif()

