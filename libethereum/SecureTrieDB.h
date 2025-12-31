// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libdevcore/TrieDB.h>

namespace dev
{
namespace eth
{
#if ETH_FATDB
template <class KeyType, class DB>
using SecureTrieDB = SpecificTrieDB<FatGenericTrieDB<DB>, KeyType>;
#else
template <class KeyType, class DB>
using SecureTrieDB = SpecificTrieDB<HashedGenericTrieDB<DB>, KeyType>;
#endif

}  // namespace eth
}  // namespace dev
