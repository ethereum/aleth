function(create_build_info NAME)
	# Set build platform; to be written to BuildInfo.h
	set(ETH_BUILD_PLATFORM "${CMAKE_SYSTEM_NAME}")
	if (CMAKE_COMPILER_IS_MINGW)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/mingw")
	elseif (CMAKE_COMPILER_IS_MSYS)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/msys")
	elseif (CMAKE_COMPILER_IS_GNUCXX)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/g++")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/msvc")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/clang")
	else ()
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/unknown")
	endif ()

	if (EVMJIT)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/JIT")
	else ()
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/int")
	endif ()

	if (PARANOID)
		set(ETH_BUILD_PLATFORM "${ETH_BUILD_PLATFORM}/PARA")
	endif ()

	#cmake build type may be not speCified when using msvc
	if (CMAKE_BUILD_TYPE)
		set(_cmake_build_type ${CMAKE_BUILD_TYPE})
	else()
		set(_cmake_build_type "${CMAKE_CFG_INTDIR}")
	endif()

	# Generate header file containing useful build information
	add_custom_target(${NAME}_BuildInfo.h ALL
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DETH_BUILDINFO_IN="${ETH_CMAKE_DIR}/templates/BuildInfo.h.in" -DETH_DST_DIR="${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}" -DETH_CMAKE_DIR="${ETH_CMAKE_DIR}"
			-DETH_BUILD_TYPE="${_cmake_build_type}" -DETH_BUILD_PLATFORM="${ETH_BUILD_PLATFORM}"
			-DPROJECT_VERSION="${PROJECT_VERSION}" -DETH_FATDB="${FATDB10}" -P "${ETH_SCRIPTS_DIR}/buildinfo.cmake"
		)
	include_directories(BEFORE ${PROJECT_BINARY_DIR})
endfunction()

function(create_config_info)
	# Generate header file containing configuration info
	foreach(FEATURE ${ARGN})
		if (${FEATURE})
			set(ETH_${FEATURE} 1)
		else()
			set(ETH_${FEATURE} 0)
		endif()
	endforeach()

	set(ETH_SOURCE_DIR ${PROJECT_SOURCE_DIR})
	set(ETH_DST_DIR "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}")

	set(INFILE "${ETH_SOURCE_DIR}/ConfigInfo.h.in")
	set(TMPFILE "${ETH_DST_DIR}/ConfigInfo.h.tmp")
	set(OUTFILE "${ETH_DST_DIR}/ConfigInfo.h")

	if (EXISTS ${INFILE})
		configure_file("${INFILE}" "${TMPFILE}")

		include("${ETH_CMAKE_DIR}/EthUtils.cmake")
		replace_if_different("${TMPFILE}" "${OUTFILE}" CREATE)

		include_directories(BEFORE "${PROJECT_BINARY_DIR}/include")
	endif()
endfunction()
