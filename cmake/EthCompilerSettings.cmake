#------------------------------------------------------------------------------
# EthCompilerSettings.cmake
#
# CMake file for cpp-ethereum project which specifies our compiler settings
# for each supported platform and build configuration.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# Copyright (c) 2014-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

# Clang seeks to be command-line compatible with GCC as much as possible, so
# most of our compiler settings are common between GCC and Clang.
#
# These settings then end up spanning all POSIX platforms (Linux, OS X, BSD, etc)

# Use ccache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
	message("Using ccache")
endif(CCACHE_FOUND)

if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))

	# Use ISO C++11 standard language.
	set(CMAKE_CXX_FLAGS -std=c++11)

	# Enables all the warnings about constructions that some users consider questionable,
	# and that are easy to avoid.  Also enable some extra warning flags that are not
	# enabled by -Wall.   Finally, treat at warnings-as-errors, which forces developers
	# to fix warnings as they arise, so they don't accumulate "to be fixed later".
	add_compile_options(-Wall)
	add_compile_options(-Wextra)
	add_compile_options(-Werror)

	# Disable warnings about unknown pragmas (which is enabled by -Wall).
	add_compile_options(-Wno-unknown-pragmas)

	# Configuration-specific compiler settings.
	set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g -DETH_DEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG -DETH_RELEASE")
	set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG -DETH_RELEASE")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DETH_RELEASE")

	option(USE_LD_GOLD "Use GNU gold linker" ON)
	if (USE_LD_GOLD)
		execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
		if ("${LD_VERSION}" MATCHES "GNU gold")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
		endif ()
	endif ()

	# Additional GCC-specific compiler settings.
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

		# Check that we've got GCC 4.7 or newer.
		execute_process(
			COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
		if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
			message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
		endif ()

		# Strong stack protection was only added in GCC 4.9.
		# Use it if we have the option to do so.
		# See https://lwn.net/Articles/584225/
		if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
			add_compile_options(-fstack-protector-strong)
			add_compile_options(-fstack-protector)
		endif()

	# Additional Clang-specific compiler settings.
	elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

		add_compile_options(-fstack-protector)

		# Enable strong stack protection only on Mac and only for OS X Yosemite
		# or newer (AppleClang 7.0+).  We should be able to re-enable this setting
		# on non-Apple Clang as well, if we can work out what expression to use for
		# the version detection.

		# The fact that the version-reporting for AppleClang loses the original
		# Clang versioning is rather annoying.  Ideally we could just have
		# a single cross-platform "if version >= 3.4.1" check.
		#
		# There is debug text in the else clause below, to help us work out what
		# such an expression should be, if we can get this running on a Trusty box
		# with Clang.  Greg Colvin previously replicated the issue there too.
		#
		# See https://github.com/ethereum/webthree-umbrella/issues/594

		if (APPLE)
			if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
				add_compile_options(-fstack-protector-strong)
			endif()
		else()
			message(WARNING "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
		endif()

		# Some Linux-specific Clang settings.  We don't want these for OS X.
		if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

			# Tell Boost that we're using Clang's libc++.   Not sure exactly why we need to do.
			add_definitions(-DBOOST_ASIO_HAS_CLANG_LIBCXX)

			# Use fancy colors in the compiler diagnostics
			add_compile_options(-fcolor-diagnostics)
		endif()
	endif()

# The major alternative compiler to GCC/Clang is Microsoft's Visual C++ compiler, only available on Windows.
elseif (MSVC)

    add_compile_options(/MP)						# enable parallel compilation
	add_compile_options(/EHsc)						# specify Exception Handling Model in msvc
	add_compile_options(/WX)						# enable warnings-as-errors
	add_compile_options(/wd4068)					# disable unknown pragma warning (4068)
	add_compile_options(/wd4996)					# disable unsafe function warning (4996)
	add_compile_options(/wd4503)					# disable decorated name length exceeded, name was truncated (4503)
	add_compile_options(/wd4267)					# disable conversion from 'size_t' to 'type', possible loss of data (4267)
	add_compile_options(/wd4180)					# disable qualifier applied to function type has no meaning; ignored (4180)
	add_compile_options(/wd4290)					# disable C++ exception specification ignored except to indicate a function is not __declspec(nothrow) (4290)
	add_compile_options(/wd4297)					# disable <vector>'s function assumed not to throw an exception but does (4297)
	add_compile_options(/wd4244)					# disable conversion from 'type1' to 'type2', possible loss of data (4244)
	add_compile_options(/wd4800)					# disable forcing value to bool 'true' or 'false' (performance warning) (4800)
	add_compile_options(-D_WIN32_WINNT=0x0600)		# declare Windows Vista API requirement
	add_compile_options(-DNOMINMAX)					# undefine windows.h MAX && MIN macros cause it cause conflicts with std::min && std::max functions
	add_compile_options(-DMINIUPNP_STATICLIB)		# define miniupnp static library

	# Always use Release variant of C++ runtime.
	# We don't want to provide Debug variants of all dependencies. Some default
	# flags set by CMake must be tweaked.
	string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	string(REPLACE "/MDd" "/MD" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	string(REPLACE "/D_DEBUG" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS OFF)

	# disable empty object file warning
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
	# warning LNK4075: ignoring '/EDITANDCONTINUE' due to '/SAFESEH' specification
	# warning LNK4099: pdb was not found with lib
	# stack size 16MB
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099,4075 /STACK:16777216")

# If you don't have GCC, Clang or VC++ then you are on your own.  Good luck!
else ()
	message(WARNING "Your compiler is not tested, if you run into any issues, we'd welcome any patches.")
endif ()

if (SANITIZE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=${SANITIZE}")
	if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
		set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/sanitizer-blacklist.txt")
	endif()
endif()

if (PROFILING AND (("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")))
	set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-g ${CMAKE_C_FLAGS}")
	add_definitions(-DETH_PROFILING_GPERF)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lprofiler")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lprofiler")
endif ()

if (COVERAGE)
	set(CMAKE_CXX_FLAGS "-g --coverage ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-g --coverage ${CMAKE_C_FLAGS}")
	set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
	find_program(LCOV_TOOL lcov)
	message(STATUS "lcov tool: ${LCOV_TOOL}")
	if (LCOV_TOOL)
		add_custom_target(coverage.info
			COMMAND ${LCOV_TOOL} -o ${CMAKE_BINARY_DIR}/coverage.info -c -d ${CMAKE_BINARY_DIR}
			COMMAND ${LCOV_TOOL} -o ${CMAKE_BINARY_DIR}/coverage.info -r ${CMAKE_BINARY_DIR}/coverage.info '/usr*' '${CMAKE_BINARY_DIR}/deps/*' '${CMAKE_SOURCE_DIR}/deps/*'
		)
	endif()
endif ()
