# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

# This module, when included, sets default values for params related to
# Hunter cache servers, including upload options.

# Default Hunter cache servers.
set(HUNTER_CACHE_SERVERS
    "https://github.com/ethereum/hunter-cache;https://github.com/ingenue/hunter-cache"
    CACHE STRING "Hunter cache servers")

# Default path to Hunter passwords file containing information how to access
# Ethereum's cache server.
set(HUNTER_PASSWORDS_PATH
    ${CMAKE_CURRENT_LIST_DIR}/HunterCacheServers-passwords.cmake
    CACHE STRING "Hunter passwords file")

# In CI builds upload the binaries if the HUNTER_CACHE_TOKEN was decrypted
# (only for branches and internal PRs).
if("$ENV{CI}" AND NOT "$ENV{HUNTER_CACHE_TOKEN}" STREQUAL "")
    set(run_upload YES)
else()
    set(run_upload NO)
endif()
option(HUNTER_RUN_UPLOAD "Upload binaries to the Hunter cache server" ${run_upload})
unset(run_upload)
