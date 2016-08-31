#------------------------------------------------------------------------------
# CMake helper for libdevcore, libdevcrypto and libp2p modules.
#
# This module defines
#     Dev_XXX_LIBRARIES, the libraries needed to use ethereum.
#     Dev_INCLUDE_DIRS
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
set(LIBS devcore;devcrypto;p2p)

set(Dev_INCLUDE_DIRS "${CPP_ETHEREUM_DIR}")

# if the project is a subset of main cpp-ethereum project
# use same pattern for variables as Boost uses
if ((DEFINED cpp-ethereum_VERSION) OR (DEFINED dev_VERSION))

	foreach (l ${LIBS}) 
		string(TOUPPER ${l} L)
		set ("Dev_${L}_LIBRARIES" ${l})
	endforeach()

else()

	foreach (l ${LIBS})
		string(TOUPPER ${l} L)

		find_library(Dev_${L}_LIBRARY
			NAMES ${l}
			PATHS ${CMAKE_LIBRARY_PATH}
			PATH_SUFFIXES "lib${l}" "${l}" "lib${l}/Debug" "lib${l}/Release" 
			NO_DEFAULT_PATH
		)

		set(Dev_${L}_LIBRARIES ${Dev_${L}_LIBRARY})

		if (MSVC)
			find_library(Dev_${L}_LIBRARY_DEBUG
				NAMES ${l}
				PATHS ${CMAKE_LIBRARY_PATH}
				PATH_SUFFIXES "lib${l}/Debug" 
				NO_DEFAULT_PATH
			)
			eth_check_library_link(Dev_${L})
		endif()
	endforeach()

endif()

