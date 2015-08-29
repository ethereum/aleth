function(eth_apply TARGET REQUIRED SUBMODULE)
	# TODO take into account REQUIRED

	if (DEFINED ethereum_SOURCE_DIR)
		set(ETH_DIR ${ethereum_SOURCE_DIR})
		set(ETH_BUILD_DIR ${ethereum_BINARY_DIR})
	else ()
		set(ETH_DIR             "${PROJECT_SOURCE_DIR}/../cpp-ethereum"			CACHE PATH "The path to the cpp-ethereum directory")
		set(ETH_BUILD_DIR_NAME  "build"                                     	CACHE STRING "The name of the build directory in cpp-ethereum")
		set(ETH_BUILD_DIR       "${ETH_DIR}/${ETH_BUILD_DIR_NAME}")
		set(CMAKE_LIBRARY_PATH 	${ETH_BUILD_DIR};${CMAKE_LIBRARY_PATH})
	endif()

	find_package(Eth)
	target_include_directories(${TARGET} PUBLIC ${ETH_DIR})
	# look for Buildinfo.h in build dir
	target_include_directories(${TARGET} PUBLIC ${ETH_BUILD_DIR})

	# Base is where all dependencies for devcore are
	if (${SUBMODULE} STREQUAL "base")

		# if it's ethereum source dir, alwasy build BuildInfo.h before
		if (DEFINED ethereum_SOURCE_DIR)
			add_dependencies(${TARGET} BuildInfo.h)
		endif()

		target_include_directories(${TARGET} SYSTEM BEFORE PUBLIC ${JSONCPP_INCLUDE_DIRS})	
		target_include_directories(${TARGET} BEFORE PUBLIC ..)	
		target_include_directories(${TARGET} SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})	
		target_include_directories(${TARGET} SYSTEM PUBLIC ${DB_INCLUDE_DIRS})	

		target_link_libraries(${TARGET} ${Boost_THREAD_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_RANDOM_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_FILESYSTEM_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_SYSTEM_LIBRARIES})
		target_link_libraries(${TARGET} ${JSONCPP_LIBRARIES})
		target_link_libraries(${TARGET} ${DB_LIBRARIES})

		if (DEFINED MSVC)
			target_link_libraries(${TARGET} ${Boost_CHRONO_LIBRARIES})
			target_link_libraries(${TARGET} ${Boost_DATE_TIME_LIBRARIES})
		endif()

		if (NOT DEFINED MSVC)
			target_link_libraries(${TARGET} pthread)
		endif()
	endif()

	if (${SUBMODULE} STREQUAL "devcore")
		eth_use(${TARGET} ${REQUIRED} Eth::base)
		target_link_libraries(${TARGET} ${Eth_DEVCORE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "scrypt")
		target_link_libraries(${TARGET} ${Eth_SCRYPT_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "secp256k1")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${GMP_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${GMP_LIBRARIES})
		target_link_libraries(${TARGET} ${Eth_SECP256K1_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC ETH_HAVE_SECP256K1)
	endif()

	if (${SUBMODULE} STREQUAL "devcrypto")
		eth_use(${TARGET} ${REQUIRED} Eth::devcore Eth::scrypt Cryptopp)
		if (NOT DEFINED MSVC)
			eth_use(${TARGET} ${REQUIRED} Eth::secp256k1)
		endif()

		target_link_libraries(${TARGET} ${Eth_DEVCRYPTO_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "ethash")
		# even if ethash is required, Cryptopp is optional
		eth_use(${TARGET} OPTIONAL Cryptopp)
		target_link_libraries(${TARGET} ${Eth_ETHASH_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "ethash-cl")
		if (ETHASHCL)
			if (OpenCL_FOUND)
				eth_use(${TARGET} ${REQUIRED} Eth::ethash)
				target_include_directories(${TARGET} SYSTEM PUBLIC ${OpenCL_INCLUDE_DIRS})
				target_link_libraries(${TARGET} ${OpenCL_LIBRARIES})
				target_link_libraries(${TARGET} ${Eth_ETHASH-CL_LIBRARIES})
				eth_copy_dlls(${TARGET} OpenCL_DLLS)
			elseif (${REQUIRED} STREQUAL "REQUIRED")
				message(FATAL_ERROR "OpenCL library was not found")
			endif()
		endif()
	endif()

	if (${SUBMODULE} STREQUAL "ethcore")
		eth_use(${TARGET} ${REQUIRED} Eth::devcrypto)
		# even if ethcore is required, ethash-cl and cpuid are optional
		eth_use(${TARGET} OPTIONAL Eth::ethash-cl Cpuid)
		target_link_libraries(${TARGET} ${Eth_ETHCORE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "p2p")
		eth_use(${TARGET} ${REQUIRED} Eth::devcore Eth::devcrypto)
		eth_use(${TARGET} OPTIONAL Miniupnpc)
		target_link_libraries(${TARGET} ${Eth_P2P_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "evmcore")
		eth_use(${TARGET} ${REQUIRED} Eth::devcore)
		target_link_libraries(${TARGET} ${Eth_EVMCORE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "evmjit")
		# TODO: not sure if should use evmjit and/or evmjit-cpp
		# TODO: take into account REQUIRED variable
		if (EVMJIT)
			target_link_libraries(${TARGET} ${Eth_EVMJIT_LIBRARIES})
			target_link_libraries(${TARGET} ${Eth_EVMJIT-CPP_LIBRARIES})
		endif()
	endif()

	if (${SUBMODULE} STREQUAL "evm")
		eth_use(${TARGET} ${REQUIRED} Eth::ethcore Eth::devcrypto Eth::evmcore Eth::devcore)
		eth_use(${TARGET} OPTIONAL Eth::evmjit)
		target_link_libraries(${TARGET} ${Eth_EVM_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "evmasm")
		eth_use(${TARGET} ${REQUIRED} Eth::evmcore)
		target_link_libraries(${TARGET} ${Eth_EVMASM_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "lll")
		eth_use(${TARGET} ${REQUIRED} Eth::evmasm)
		target_link_libraries(${TARGET} ${Eth_LLL_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "ethereum")
		eth_use(${TARGET} ${REQUIRED} Eth::evm Eth::lll Eth::p2p Eth::devcrypto Eth::ethcore JsonRpc::Server JsonRpc::Client)
		target_link_libraries(${TARGET} ${Boost_REGEX_LIBRARIES})
		target_link_libraries(${TARGET} ${Eth_ETHEREUM_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "jsengine")
		eth_use(${TARGET} ${REQUIRED} V8)
		target_link_libraries(${TARGET} ${Eth_JSENGINE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "jsconsole")
		eth_use(${EXECUTABLE} ${REQUIRED} Eth::jsengine Eth::devcore JsonRpc::Server JsonRpc::Client)
		eth_use(${EXECUTABLE} OPTIONAL Readline)
		target_link_libraries(${TARGET} ${Eth_JSCONSOLE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "natspec")
		target_link_libraries(${TARGET} ${Eth_NATSPEC_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "testutils")
		eth_use(${EXECUTABLE} ${REQUIRED} Eth::ethereum)
		target_link_libraries(${TARGET} ${Eth_TESTUTILS_LIBRARIES})
	endif()

endfunction()
