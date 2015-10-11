cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../webthree-helpers/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(dev VERSION "0.9.94")

include(EthCompilerSettings)

include(EthExecutableHelper)

include(EthDependencies)

include(EthUtils)

include(EthOptions)
configure_project(PARANOID TOOLS TESTS MINIUPNPC)

add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
if (NOT EMSCRIPTEN)
	add_subdirectory(libp2p)

	if (TOOLS)
		add_subdirectory(rlp)
	endif()
endif()

if (TESTS)
	add_subdirectory(test)
endif()
