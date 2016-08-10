# - Try to find the ssh2 libssh2 library
# Once done this will define
#
#  SSH2_FOUND - system has the SSH2 libssh2 library
#  SSH2_INCLUDE_DIR - the SSH2 libssh2 include directory
#  SSH2_LIBRARIES - The libraries needed to use SSH2 libssh2

# only look in default directories
find_path(
	SSH2_INCLUDE_DIR
	NAMES libssh2.h
	DOC "ssh2 include dir"
)

find_library(
	SSH2_LIBRARY
	NAMES ssh2 libssh2
	DOC "ssh2 library"
)

set(SSH2_INCLUDE_DIRS ${SSH2_INCLUDE_DIR})
set(SSH2_LIBRARIES ${SSH2_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SSH2 DEFAULT_MSG
	SSH2_LIBRARY SSH2_INCLUDE_DIR)
mark_as_advanced (SSH2_INCLUDE_DIR SSH2_LIBRARY)

