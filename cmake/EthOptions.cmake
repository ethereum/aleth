macro(configure_project)
	set(NAME ${PROJECT_NAME})

	# Default to RelWithDebInfo configuration if no configuration is explicitly specified.
	if (NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
			"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
	endif()
	
	# features
	eth_default_option(VMTRACE OFF)
	eth_default_option(PROFILING OFF)
	eth_default_option(FATDB ON)
	eth_default_option(ROCKSDB OFF)
	eth_default_option(OLYMPIC OFF)
	eth_default_option(PARANOID OFF)
	eth_default_option(MINIUPNPC ON)

	# BuildBuild, AKA Unity Builds is a hack for C++ builds which is very well known within
	# the video-games industry, but is perhaps less known in other parts of the software
	# industry.
	#
	# It is a huge optimization for clean build times, but can also result in better
	# generated code, because the compiler gets a larger set of code to optimize in each
	# pass.  This is much the same as Link Time Optimization (LTO) in GCC and
	# Link Time Code Generation (LTCG) and Whole Program Optimization in VC++.
	#
	# C++ uses the same compilation-unit model brought forward from C where there is a very
	# crude and time-expensive pre-processing phase on each source file, resulting in
	# compilation to an object file.  Those object files are then later maybe archived into
	# libraries, and ultimately linked together into an executable.
	#
	# That pre-processing phase is commonly what takes the bulk of the time, because the
	# same header files are repeatedly being read from disk and expanded.  Large C++
	# applications have hundreds or thousands of source files, and the graph of header
	# file dependencies for them all will have a lot of commonality, not of which can be
	# re-used with a normal development flow.  Pre-compiled headers are an attempt to fix
	# this problem, but they are not a standard feature, and often require a lot of very
	# brittle manual configuration.
	#
	# The majestic hack here is to combine multiple compilation units within the build
	# system before they ever get passed to the compiler.  That just means dynamic
	# generation of a BulkBuild source file which just directly includes all the source
	# files for a given library or application, so they all get processed as part of a
	# single pass, so that all the header commonality can be optimized out, and there
	# is no redundant reprocessing.
	#
	# That often "just works".  Sometimes clashing static symbols need tweaking.  You
	# can have a scenario where such symbols were using file-level scoping rather
	# than namespace scoping and are now forced together into a single compilation
	# unit, so now they clash.  Similar clashes are possible for 'using namespace'.
	# This is easy stuff to address.
	#
	# Another possible variant, which we aren't doing here yet, but maybe will want to
	# do so soon is NOT to include all source files in a single unit, but to have a
	# number of "units" for related subsets of code which are often modified together.
	#
	# There is a straight tradeoff also been clean build times (which can be 10x faster
	# with BulkBuild), and incremental build times (which can be a bit slower with
	# BulkBuild, because the large unit of code needs rebuilding whenever anything
	# changes).
	#
	# Another downside to BulkBuild, though one which is easy to mitigate against, is
	# dependencies-rot.  If you only ever build with BulkBuild enabled then you can
	# get includes indirectly through source files processed earlier with the
	# BulkBuild unit, and find that you code doesn't build anymore "loose" because
	# you are missing includes, but got away with it because of BulkBuild already
	# having processed those headers.
	#
	# The easy mitigation to this issue is to support both "loose" and BulkBuild
	# modes in your build system, and to ensure that you build both modes within
	# your automation builds.   Engineers will commonly run with BulkBuild enabled
	# at all times for their local builds, with automated builds ensuring that
	# any dependencies rot which is introduced is detected in a timely manner,
	# and then can be addressed.
	eth_default_option(BULK_BUILD ON)

	# components
	eth_default_option(TESTS ON)
	eth_default_option(TOOLS ON)
	eth_default_option(ETHASHCL ON)
	eth_default_option(EVMJIT OFF)
	eth_default_option(SOLIDITY ON)

	# Resolve any clashes between incompatible options.
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		if (PARANOID)
			message("Paranoia requires debug - disabling for release build.")
			set(PARANOID OFF)
		endif ()
		if (VMTRACE)
			message("VM Tracing requires debug - disabling for release build.")
			set (VMTRACE OFF)
		endif ()
	endif ()

	# Define a matching property name of each of the "features".
	foreach(FEATURE ${ARGN})
		set(SUPPORT_${FEATURE} TRUE)
	endforeach()

	# Temporary pre-processor symbol used for hiding broken unit-tests.
	# Hiding them behind this pre-processor symbol lets us turn them off
	# and on again easily enough, and also to grep for them.
	add_definitions(-DDISABLE_BROKEN_UNIT_TESTS_UNTIL_WE_FIX_THEM)

	# TODO:  Eliminate this pre-processor symbol, which is a bad pattern.
	# Common code has no business knowing which application it is part of.
	add_definitions(-DETH_TRUE)

	# TODO:  Why do we even bother with Olympic support anymore?
	if (OLYMPIC)
		add_definitions(-DETH_OLYMPIC)
	else()
		add_definitions(-DETH_FRONTIER)
	endif()

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

	# TODO:  What does "paranoia" even mean?
	if (PARANOID)
		add_definitions(-DETH_PARANOIA)
	endif ()

	# TODO:  What does "VM trace" even mean?
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
	create_build_info(${NAME})
	print_config(${NAME})
endmacro()

macro(print_config NAME)
	message("")
	message("------------------------------------------------------------------------")
	message("-- Configuring ${NAME}")
	message("------------------------------------------------------------------------")
	message("--                  CMake Version                            ${CMAKE_VERSION}")
	message("-- CMAKE_BUILD_TYPE Build type                               ${CMAKE_BUILD_TYPE}")
	message("-- BULK_BUILD       BulkBuild enabled?                       ${BULK_BUILD}")
	message("-- TARGET_PLATFORM  Target platform                          ${CMAKE_SYSTEM_NAME}")
	message("--------------------------------------------------------------- features")
if (SUPPORT_CPUID)
	message("--                  Hardware identification support          ${CPUID_FOUND}")
endif()
if (SUPPORT_CURL)
	message("--                  HTTP Request support                     ${CURL_FOUND}")
endif()
if (SUPPORT_VMTRACE)
	message("-- VMTRACE          VM execution tracing                     ${VMTRACE}")
endif()
	message("-- PROFILING        Profiling support                        ${PROFILING}")
if (SUPPORT_FATDB)
	message("-- FATDB            Full database exploring                  ${FATDB}")
endif()
if (SUPPORT_ROCKSDB)
	message("-- ROCKSDB          Prefer rocksdb to leveldb                ${ROCKSDB}")
endif()
if (SUPPORT_OLYMPIC)
	message("-- OLYMPIC          Default to the Olympic network           ${OLYMPIC}")
endif()
if (SUPPORT_PARANOID)
	message("-- PARANOID         -                                        ${PARANOID}")
endif()
if (SUPPORT_MINIUPNPC)
	message("-- MINIUPNPC        -                                        ${MINIUPNPC}")
endif()
	message("------------------------------------------------------------- components")
if (SUPPORT_TESTS)
	message("-- TESTS            Build tests                              ${TESTS}")
endif()
if (SUPPORT_TOOLS)
	message("-- TOOLS            Build tools                              ${TOOLS}")
endif()
if (SUPPORT_ETHASHCL)
	message("-- ETHASHCL         Build OpenCL components                  ${ETHASHCL}")
endif()
if (SUPPORT_EVMJIT)
	message("-- EVMJIT           Build LLVM-based JIT EVM                 ${EVMJIT}")
endif()
if (SUPPORT_SOLIDITY)
	message("-- SOLIDITY         Build Solidity                           ${SOLIDITY}")
endif()
	message("------------------------------------------------------------------------")
	message("")
endmacro()
