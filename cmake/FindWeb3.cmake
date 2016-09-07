#------------------------------------------------------------------------------
# CMake helper for libwhisper, libwebthree and libweb3jsonrpc.
#
# This module defines
#     Web3_XXX_LIBRARIES, the libraries needed to use ethereum.
#     TODO: Web3_INCLUDE_DIRS
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2014-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

include(EthUtils)
set(LIBS whisper;webthree;web3jsonrpc)

set(Web3_INCLUDE_DIRS ${CPP_ETHEREUM_DIR})

# if the project is a subset of cpp-ethereum
# use same pattern for variables as Boost uses
if ((DEFINED webthree_VERSION) OR (DEFINED cpp-ethereum_VERSION))

	foreach (l ${LIBS})
		string(TOUPPER ${l} L)
		set ("Web3_${L}_LIBRARIES" ${l})
	endforeach()

else()

	foreach (l ${LIBS})
		string(TOUPPER ${l} L)

		find_library(Web3_${L}_LIBRARY
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Release"
			NO_DEFAULT_PATH
		)

		set(Web3_${L}_LIBRARIES ${Web3_${L}_LIBRARY})

		if (MSVC)
			find_library(Web3_${L}_LIBRARY_DEBUG
				NAMES ${l}
				PATHS ${CMAKE_LIBRARY_PATH}
				PATH_SUFFIXES "lib${l}/Debug" 
				NO_DEFAULT_PATH
			)
			eth_check_library_link(Web3_${L})
		endif()
	endforeach()

endif()
