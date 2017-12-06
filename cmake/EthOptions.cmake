macro(configure_project)
	# Default to RelWithDebInfo configuration if no configuration is explicitly specified.
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	   set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type on single-configuration generators" FORCE)
	endif()
	set(TESTETH_ARGS "" CACHE STRING "Testeth arguments for ctest tests")

	option(BUILD_SHARED_LIBS "Build project libraries shared" OFF)

	# Features:
	option(VMTRACE "Enable VM tracing" OFF)
	option(PROFILING "Enable profiling (deprecated)" OFF)
	option(FATDB "Enable fat state database" ON)
	option(PARANOID "Enable additional checks when validating transactions (deprecated)" OFF)
	option(MINIUPNPC "Build with UPnP support" OFF)
	option(FASTCTEST "Enable fast ctest" OFF)

	set(supported_dbs LEVELDB ROCKSDB LMDB)
	string(REPLACE ";" ", " supported_dbs_str "${supported_dbs}")
	set(DB LEVELDB CACHE STRING "Database implementation to use (${supported_dbs_str})")
	set_property(CACHE DB PROPERTY STRINGS ${supported_dbs})
	set(valid_db OFF)
	foreach(supported_db ${supported_dbs})
		if(DB STREQUAL supported_db)
			set(valid_db ON)
			break()
		endif()
	endforeach()
	if(NOT valid_db)
		message(WARNING
			"Invalid database selected ${DB}, must be one of"
			" ${supported_dbs_str}, using LEVELDB instead")
		set(DB LEVELDB CACHE STRING
			  "Database implementation to use (${supported_dbs_str})"
				FORCE)
	endif()

	add_definitions("-DETH_${DB}")
	if(HUNTER_ENABLED)
		string(TOLOWER "${DB}" hunter_name)
		hunter_add_package(${hunter_name})

		if(DB STREQUAL LMDB)
			find_package(liblmdb CONFIG REQUIRED)
		elseif(DB STREQUAL ROCKSDB)
			find_package(RocksDB CONFIG REQUIRED)
		else()
			find_package(leveldb CONFIG REQUIRED)
		endif()
	else()
		if(DB STREQUAL LMDB)
			find_package(lmdb REQUIRED)
		elseif(DB STREQUAL ROCKSDB)
			find_package(RocksDB REQUIRED)
		else()
			find_package(LevelDB REQUIRED)
		endif()
	endif()

	if(MINIUPNPC)
		message(WARNING
			"Security vulnerabilities have been discovered in miniupnpc library. "
			"This build option is for testing only. Do not use it in public networks")
	endif()

	# components
	option(TESTS "Build with tests" ON)
	option(TOOLS "Build additional tools" ON)
	option(EVMJIT "Build with EVMJIT module enabled" OFF)

	# Resolve any clashes between incompatible options.
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		if (PARANOID)
			message(WARNING "Paranoia requires debug - disabling for release build.")
			set(PARANOID OFF)
		endif ()
		if (VMTRACE)
			message(WARNING "VM Tracing requires debug - disabling for release build.")
			set (VMTRACE OFF)
		endif ()
	endif ()

	# Are we including the JIT EVM module?
	# That pulls in a quite heavyweight LLVM dependency, which is
	# not suitable for all platforms.
	if (EVMJIT)
		add_definitions(-DETH_EVMJIT)
	endif ()

	# FATDB is an option to include the reverse hashes for the trie,
	# i.e. it allows you to iterate over the contents of the state.
	if (FATDB)
		add_definitions(-DETH_FATDB)
	endif ()

	if (PARANOID)
		add_definitions(-DETH_PARANOIA)
	endif ()

	if (VMTRACE)
		add_definitions(-DETH_VMTRACE)
	endif ()

	# CI Builds should provide (for user builds this is totally optional)
	# -DBUILD_NUMBER - A number to identify the current build with. Becomes TWEAK component of project version.
	# -DVERSION_SUFFIX - A string to append to the end of the version string where applicable.
	if (NOT DEFINED BUILD_NUMBER)
		# default is big so that local build is always considered greater
		# and can easily replace CI build for for all platforms if needed.
		# Windows max version component number is 65535
		set(BUILD_NUMBER 65535)
	endif()

	# Suffix like "-rc1" e.t.c. to append to versions wherever needed.
	if (NOT DEFINED VERSION_SUFFIX)
		set(VERSION_SUFFIX "")
	endif()

	include(EthBuildInfo)
	create_build_info()
	print_config()
endmacro()

macro(print_config)
	message("")
	message("------------------------------------------------------------------------")
	message("-- Configuring ${PROJECT_NAME}")
	message("------------------------------------------------------------------------")
	message("-- CMake ${CMAKE_VERSION} (${CMAKE_COMMAND})")
	message("-- CMAKE_BUILD_TYPE Build type                               ${CMAKE_BUILD_TYPE}")
	message("-- TARGET_PLATFORM  Target platform                          ${CMAKE_SYSTEM_NAME}")
	message("-- BUILD_SHARED_LIBS                                         ${BUILD_SHARED_LIBS}")
	message("--------------------------------------------------------------- features")
	message("-- VMTRACE          VM execution tracing                     ${VMTRACE}")
	message("-- PROFILING        Profiling support                        ${PROFILING}")
	message("-- FATDB            Full database exploring                  ${FATDB}")
	message("-- DB               Database implementation                  ${DB}")
	message("-- PARANOID         -                                        ${PARANOID}")
	message("-- MINIUPNPC        -                                        ${MINIUPNPC}")
	message("------------------------------------------------------------- components")
	message("-- TESTS            Build tests                              ${TESTS}")
	message("-- TOOLS            Build tools                              ${TOOLS}")
	message("-- EVMJIT           Build LLVM-based JIT EVM                 ${EVMJIT}")
	message("------------------------------------------------------------- tests")
	message("-- FASTCTEST        Run only test suites in ctest            ${FASTCTEST}")
	message("-- TESTETH_ARGS     Testeth arguments in ctest:               ")
	message("                    ${TESTETH_ARGS}")
	message("------------------------------------------------------------------------")
	message("")
endmacro()
