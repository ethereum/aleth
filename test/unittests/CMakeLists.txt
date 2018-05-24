# Aleth: Ethereum C++ client and tools.
# Copyright 2018 Aleth Authors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

set(sources
    main.cpp
    libethashseal/EthashTest.cpp
)

add_executable(aleth-test ${sources})
target_link_libraries(aleth-test PRIVATE ethashseal)

# Temporary for JSON spirit.
#target_include_directories(aleth-test PRIVATE ${UTILS_INCLUDE_DIR})
set_target_properties(aleth-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
