// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>

namespace dev
{

class MemTrieNode;

/**
 * @brief Merkle Patricia Tree "Trie": a modifed base-16 Radix tree.
 */
class MemTrie
{
public:
	MemTrie(): m_root(nullptr) {}
	~MemTrie();

	h256 hash256() const;
	bytes rlp() const;

	void debugPrint();

	std::string const& at(std::string const& _key) const;
	void insert(std::string const& _key, std::string const& _value);
	void remove(std::string const& _key);

private:
	MemTrieNode* m_root;
};

}
