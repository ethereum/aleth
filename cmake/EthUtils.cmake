#
# renames the file if it is different from its destination
include(CMakeParseArguments)
#
macro(replace_if_different SOURCE DST)
	set(extra_macro_args ${ARGN})
	set(options CREATE)
	set(one_value_args)
	set(multi_value_args)
	cmake_parse_arguments(REPLACE_IF_DIFFERENT "${options}" "${one_value_args}" "${multi_value_args}" "${extra_macro_args}")

	if (REPLACE_IF_DIFFERENT_CREATE AND (NOT (EXISTS "${DST}")))
		file(WRITE "${DST}" "")
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${SOURCE}" "${DST}" RESULT_VARIABLE DIFFERENT OUTPUT_QUIET ERROR_QUIET)

	if (DIFFERENT)
		execute_process(COMMAND ${CMAKE_COMMAND} -E rename "${SOURCE}" "${DST}")
	else()
		execute_process(COMMAND ${CMAKE_COMMAND} -E remove "${SOURCE}")
	endif()
endmacro()

macro(eth_add_test NAME) 

	# parse arguments here
	set(commands)
	set(current_command "")
	foreach (arg ${ARGN})
		if (arg STREQUAL "ARGS")
			if (current_command)
				list(APPEND commands ${current_command})
			endif()
			set(current_command "")
		else ()
			set(current_command "${current_command} ${arg}")
		endif()
	endforeach(arg)
	list(APPEND commands ${current_command})

	message(STATUS "test: ${NAME} | ${commands}")

	# create tests
	set(index 0)
	list(LENGTH commands count)
	while (index LESS count)
		list(GET commands ${index} test_arguments)

		set(run_test "--run_test=${NAME}")
		add_test(NAME "${NAME}.${index}" COMMAND testeth ${run_test} ${test_arguments})
		
		math(EXPR index "${index} + 1")
	endwhile(index LESS count)

	# add target to run them
	add_custom_target("test.${NAME}"
		DEPENDS testeth
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -DETH_TEST_NAME="${NAME}" -DCTEST_COMMAND="${CTEST_COMMAND}" -P "${ETH_SCRIPTS_DIR}/runtest.cmake"
	)

endmacro()

# Creates C resources file from files
function(eth_add_resources RESOURCE_FILE OUT_FILE ETH_RES_DIR)
	include("${RESOURCE_FILE}")
	set(OUTPUT  "${ETH_RESOURCE_LOCATION}/${ETH_RESOURCE_NAME}.hpp")
	#message(FATAL_ERROR "res:! ${ETH_RESOURCE_LOCATION}")
	include_directories("${ETH_RESOURCE_LOCATION}")
	set(${OUT_FILE} "${OUTPUT}"  PARENT_SCOPE)

	set(filenames "${RESOURCE_FILE}")
	list(APPEND filenames "${ETH_SCRIPTS_DIR}/resources.cmake")
	foreach(resource ${ETH_RESOURCES})
		list(APPEND filenames "${${resource}}")
	endforeach(resource)

	add_custom_command(OUTPUT ${OUTPUT}
		COMMAND ${CMAKE_COMMAND} -DETH_RES_FILE="${RESOURCE_FILE}" -DETH_RES_DIR="${ETH_RES_DIR}"  -P "${ETH_SCRIPTS_DIR}/resources.cmake"
		DEPENDS ${filenames}
	)
endfunction()

macro(eth_default_option O DEF)
	if (DEFINED ${O})
		if (${${O}})
			set(${O} ON)
		else ()
			set(${O} OFF)
		endif()
	else ()
		set(${O} ${DEF})
	endif()
endmacro()

# In Windows split repositories build we need to be checking whether or not
# Debug/Release or both versions were built for the config phase to run smoothly
macro(eth_check_library_link L)
	if (${${L}_LIBRARY} AND ${${L}_LIBRARY} EQUAL "${L}_LIBRARY-NOTFOUND")
		unset(${${L}_LIBRARY})
	endif()
	if (${${L}_LIBRARY_DEBUG} AND ${${L}_LIBRARY_DEBUG} EQUAL "${L}_LIBRARY_DEBUG-NOTFOUND")
		unset(${${L}_LIBRARY_DEBUG})
	endif()
	if (${${L}_LIBRARY} AND ${${L}_LIBRARY_DEBUG})
		set(${L}_LIBRARIES optimized ${${L}_LIBRARY} debug ${${L}_LIBRARY_DEBUG})
	elseif (${${L}_LIBRARY})
		set(${L}_LIBRARIES ${${L}_LIBRARY})
	elseif (${${L}_LIBRARY_DEBUG})
		set(${L}_LIBRARIES ${${L}_LIBRARY_DEBUG})
	endif()
endmacro()

# Function which dynamically generates BulkBuild files, which are single source files which include
# all the source files for a particular library or application.   This is a hack/optimization for
# build times, and can also result in better code generation in some cases.
#
# See EthOptions.cmake file for more detailed information in this feature.

function(enable_bulkbuild BB_UNIT_NAME SOURCE_VARIABLE_NAME)
	set(files ${${SOURCE_VARIABLE_NAME}})
	set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/BulkBuild_${BB_UNIT_NAME}.cpp)
	# Tag the original source files as "header only" within CMake.
	set_source_files_properties(${files} PROPERTIES HEADER_FILE_ONLY true)
	# Open the ub file
	FILE(WRITE ${unit_build_file} "// This is a generated file\n")
	foreach(source_file ${files} )
		FILE( APPEND ${unit_build_file} "#include <${CMAKE_CURRENT_SOURCE_DIR}/${source_file}>\n")
	endforeach(source_file)
	# Add our generated BulkBuild back into the list of source files.
	if (BULK_BUILD)
		set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${unit_build_file} PARENT_SCOPE)  
	endif()
endfunction(enable_bulkbuild)
