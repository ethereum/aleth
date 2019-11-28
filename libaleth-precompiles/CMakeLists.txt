# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018-2019 Aleth Authors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

set(
    sources
    PrecompilesVM.h
    PrecompilesVM.cpp
)

set(
    dependencies
    aleth-buildinfo
    evmc::evmc
    Boost::boost
)

add_library(aleth-precompiles STATIC ${sources})
target_link_libraries(aleth-precompiles PRIVATE ${dependencies})
