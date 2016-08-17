# all dependencies that are not directly included in the cpp-ethereum distribution are defined here
# for this to work, download the dependency via the cmake script in extdep or install them manually!

function(eth_show_dependency DEP NAME)
	get_property(DISPLAYED GLOBAL PROPERTY ETH_${DEP}_DISPLAYED)
	if (NOT DISPLAYED)
		set_property(GLOBAL PROPERTY ETH_${DEP}_DISPLAYED TRUE)
		message(STATUS "${NAME} headers: ${${DEP}_INCLUDE_DIRS}")
		message(STATUS "${NAME} lib   : ${${DEP}_LIBRARIES}")
		if (NOT("${${DEP}_DLLS}" STREQUAL ""))
			message(STATUS "${NAME} dll   : ${${DEP}_DLLS}")
		endif()
	endif()
endfunction()

# The Windows platform has not historically had any standard packaging system for delivering
# versioned releases of libraries.  Homebrew and PPA perform that function for macOS and Ubuntu
# respectively, and there are analogous standards for other Linux distros.  In the absense of
# such a standard, we have chosen to make a "fake packaging system" for cpp-ethereum, which is
# implemented in https://github.com/ethereum/cpp-dependencies.
#
# NOTE - In the last couple of years, the NuGet packaging system, first created for delivery
# of .NET packages, has added support for C++ packages, and it may be possible for us to migrate
# our "fake package server" to that real package server.   That would certainly be preferable
# to rolling our own, but it also puts us at the mercy of intermediate package maintainers who
# may be inactive.  There is not a fantastic range of packages available at the time of writing,
# so we might find that such a move turns us into becoming the package maintainer for our
# dependencies.   Not a net win :-)
#
# "Windows - Try to use NuGet C++ packages"
# https://github.com/ethereum/webthree-umbrella/issues/509
#
# Perhaps a better alternative is to step away from dependencies onto binary releases entirely,
# and switching to build-from-source for some (or all) of our dependencies, especially if they
# are small.  That gives us total control, but at the cost of longer build times.  That is the
# approach which Pawel has taken for LLVM in https://github.com/ethereum/evmjit.

if (DEFINED MSVC)
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0)
		message(FATAL_ERROR "ERROR - As of the 1.3.0 release, cpp-ethereum only supports Visual Studio 2015 or newer.\nPlease download from https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx.")
	else()
		get_filename_component(DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/../deps/install" ABSOLUTE)
		set(ETH_DEPENDENCY_INSTALL_DIR
			"${DEPS_DIR}/x64"					# Old location for deps.
			"${DEPS_DIR}/win64"					# New location for deps.
			"${DEPS_DIR}/win64/Release/share"	# LLVM shared cmake files.
		)
	endif()
	set (CMAKE_PREFIX_PATH ${ETH_DEPENDENCY_INSTALL_DIR} ${CMAKE_PREFIX_PATH})
endif()

# boilerplate macros for some code editors
add_definitions(-DETH_TRUE)

# custom cmake scripts
set(ETH_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ETH_SCRIPTS_DIR ${ETH_CMAKE_DIR}/scripts)

find_program(CTEST_COMMAND ctest)

# Use Boost "multithreaded mode" for Windows.  The platform C/C++ runtime libraries come in
# two flavors on Windows, which causes an ABI schism across the whole ecosystem.  This setting
# is declaring which side of that schism we fall on.
if (DEFINED MSVC)
	set(Boost_USE_MULTITHREADED ON)
endif()

# Use the dynamic libraries for Boost for Linux and static linkage on Windows and macOS.
# We would like to use static linkage on Linux too, but on Ubuntu at least it appears that
# the prebuilt binaries for Boost won't support this.
#
# We will need to build Boost from source ourselves, with -fPIC enabled, before we are
# able to remove this conditional.  That is exactly what has been happening for months for
# the doublethinkco cross-builds (see https://github.com/doublethinkco/cpp-ethereum-cross).
#
# Typical build error we get if trying to do static Boost on Ubunty Trusty (many of them):
#
# Linking CXX shared library libdevcore.so
# /usr/bin/ld.gold: error: /usr/lib/gcc/x86_64-linux-gnu/4.8/../../../x86_64-linux-gnu/
# libboost_thread.a(thread.o): requires dynamic R_X86_64_32 reloc which may overflow at
# runtime; recompile with -fPIC
#
# https://travis-ci.org/bobsummerwill/cpp-ethereum/jobs/145955041

if (UNIX AND NOT APPLE)
	set(Boost_USE_STATIC_LIBS OFF)
else()
	set(Boost_USE_STATIC_LIBS ON)
endif()

# We have work-in-progress here to support building cpp-ethereum with static-linkage, which
# provides a fantastic end-user experience.  You have entirely standalone executables
# which are entirely atomic and "just work".  They contain everything you need and talk directly
# to the operating system kernel.  Such linkage also allow best-possible dead-code removal and
# global optimizations.  That is the model which is used by default for all golang apps.
#
# "Enable static build of binaries"
# https://github.com/ethereum/webthree-umbrella/issues/495
#
# So we just have `geth` and `geth.exe`.  This mode lets you have an `eth` which works just
# the same.  This approach is a particular benefit on Linux where there is huge fragmentation
# of packaging systems and .so versioning across distros.  Building a standalone executable
# for Linux which "just works" on all distros would be a huge win.  This mode allows just
# that to happen already on Alpine Linux using the musl standard library, and I hope we can
# extend that support to further distros in the near future.
#
# We also have partial support for static linkage on macOS, though we would only be able to
# statically link the external dependencies above the kernel.  Anything which is part of the
# operating system itself can only be used via dylibs.  Still, we can improve things a lot.
#
# This is a macOS operating system constraint.
#
# See https://developer.apple.com/library/mac/qa/qa1118/_index.html
#
# "Apple does not support statically linked binaries on Mac OS X.  A statically linked binary
# assumes binary compatibility at the kernel system call interface, and we do not make any
# guarantees on that front. Rather, we strive to ensure binary compatibility in each
# dynamically linked system library and framework."

set(STATIC_LINKING FALSE CACHE BOOL "Build static binaries")

if (STATIC_LINKING)

	set(Boost_USE_STATIC_RUNTIME ON)
	set(OpenSSL_USE_STATIC_LIBS ON)

	if (MSVC)
		# TODO - Why would we need .a on Windows?  Maybe some Cygwin-ism.
		# When I work through Windows static linkage, I will remove this,
		# if that is possible.
		set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
	elseif (APPLE)
		# At the time of writing, we are still only PARTIALLY statically linked
		# on OS X, with a mixture of statically linked external libraries where
		# those are available, and dynamically linked where that is the only
		# option we have.    Ultimately, the aim would be for everything except
		# the runtime libraries to be statically linked.
		#
		# Still TODO:
		# - jsoncpp
		# - json-rpc-cpp
		# - leveldb (which pulls in snappy, for the dylib at ;east)
		# - miniupnp
		# - gmp
		#
		# Two further libraries (curl and zlib) ship as dylibs with the platform
		# but again we could build from source and statically link these too.
		set(CMAKE_FIND_LIBRARY_SUFFIXES .a .dylib)
	else()
		set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
	endif()

	set(ETH_STATIC ON)
endif()

# Add Homebrew include paths and library paths for macOS clients.
#
# Homebrew (http://brew.sh/) is the de-facto standard package manager for Mac.
# It is a rolling-release manager, which brings its own problems, because it
# means that we are building on top of an unstable platform, where the build
# can work for me today and be broken for you tomorrow, even though we didn't
# change anything ourselves.  Qt was particularly problematic for this, but
# thankfully is no longer an issue for us anymore, after we removed the GUI
# applications in cpp-ethereum-v1.3.0.
#
# There is an alternative (minority) packaging system called MacPorts, but it isn't
# possible to run both on a single computer, so we dropped support for
# MacPorts in early 2016 to simplify our problem space.
#
# Again, as per the Windows notes above, building dependencies from source
# would sidestep these issues entirely.
#
# "Both Homebrew and MacPorts are 'rolling release' package managers"
# https://github.com/ethereum/webthree-umbrella/issues/118
if (APPLE)
	link_directories(/usr/local/lib)
	include_directories(/usr/local/include)
endif()

include_directories(BEFORE "${PROJECT_BINARY_DIR}/include")

function(eth_use TARGET REQUIRED)
	if (NOT TARGET ${TARGET})
		message(FATAL_ERROR "eth_use called for non existing target ${TARGET}")
	endif()

	if (TARGET ${PROJECT_NAME}_BuildInfo.h)
		add_dependencies(${TARGET} ${PROJECT_NAME}_BuildInfo.h)
	endif()

	foreach(MODULE ${ARGN})
		string(REPLACE "::" ";" MODULE_PARTS ${MODULE})
		list(GET MODULE_PARTS 0 MODULE_MAIN)
		list(LENGTH MODULE_PARTS MODULE_LENGTH)
		if (MODULE_LENGTH GREATER 1)
			list(GET MODULE_PARTS 1 MODULE_SUB)
		endif()
		# TODO: check if file exists if not, throws FATAL_ERROR with detailed description
		get_target_property(TARGET_APPLIED ${TARGET} TARGET_APPLIED_${MODULE_MAIN}_${MODULE_SUB})
		if (NOT TARGET_APPLIED)
			include(Use${MODULE_MAIN})
			set_target_properties(${TARGET} PROPERTIES TARGET_APPLIED_${MODULE_MAIN}_${MODULE_SUB} TRUE)
			eth_apply(${TARGET} ${REQUIRED} ${MODULE_SUB})
		endif()
	endforeach()
endfunction()
