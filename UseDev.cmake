function(eth_apply TARGET REQUIRED SUBMODULE)

	find_package(Dev)

	target_include_directories(${TARGET} BEFORE PUBLIC ${Dev_INCLUDE_DIRS})

	# Base is where all dependencies for devcore are
	if (${SUBMODULE} STREQUAL "base")
		target_include_directories(${TARGET} BEFORE PUBLIC ${CMAKE_BUILD_DIR})

		# if it's ethereum source dir, alwasy build BuildInfo.h before
		if (DEFINED dev_VERSION)
			add_dependencies(${TARGET} BuildInfo.h)
            add_dependencies(${TARGET} ConfigInfo.h)
            target_include_directories(${TARGET} BEFORE PUBLIC ${CMAKE_BINARY_DIR})
		endif()

		eth_use(${TARGET} ${REQUIRED} Jsoncpp DB::auto)
		target_include_directories(${TARGET} SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})	

		target_link_libraries(${TARGET} ${Boost_THREAD_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_RANDOM_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_FILESYSTEM_LIBRARIES})
		target_link_libraries(${TARGET} ${Boost_SYSTEM_LIBRARIES})

		if (DEFINED MSVC)
			target_link_libraries(${TARGET} ${Boost_CHRONO_LIBRARIES})
			target_link_libraries(${TARGET} ${Boost_DATE_TIME_LIBRARIES})
		endif()

		if (NOT DEFINED MSVC)
			target_link_libraries(${TARGET} pthread)
		endif()
	endif()

	if (${SUBMODULE} STREQUAL "devcore")
		eth_use(${TARGET} ${REQUIRED} Dev::base)
		target_link_libraries(${TARGET} ${Dev_DEVCORE_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "devcrypto")
		eth_use(${TARGET} ${REQUIRED} Dev::devcore Utils::scrypt Cryptopp)
		if (NOT DEFINED MSVC)
			eth_use(${TARGET} ${REQUIRED} Utils::secp256k1)
		endif()

		target_link_libraries(${TARGET} ${Dev_DEVCRYPTO_LIBRARIES})
	endif()

	if (${SUBMODULE} STREQUAL "p2p")
		eth_use(${TARGET} ${REQUIRED} Dev::devcore Dev::devcrypto)
		eth_use(${TARGET} OPTIONAL Miniupnpc)
		target_link_libraries(${TARGET} ${Eth_P2P_LIBRARIES})
	endif()

endfunction()
