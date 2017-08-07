/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file 
 *  Class for importing accounts into the State Trie
 */

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>

namespace dev
{

namespace eth
{

class StateImporter
{
public:
	explicit StateImporter(OverlayDB& _stateDb): m_trie(&_stateDb) { m_trie.init(); }

	void importAccount(h256 const& _addressHash, u256 const& _nonce, u256 const& _balance, std::map<h256, bytes> const& _storage, h256 const& _codeHash)
	{
		RLPStream s(4);
		s << _nonce << _balance;

		h256 const storageRoot = EmptyTrie;
		if (_storage.empty())
		{
			s.append(storageRoot);
		}
		else
		{
			SpecificTrieDB<GenericTrieDB<OverlayDB>, h256> storageDB(m_trie.db(), storageRoot);
			for (auto const& hashAndValue: _storage)
			{
				assert(RLP(hashAndValue.second).toInt<u256>() != 0);
/*				if (!value)
					std::cout << "Zero value in storage " << _addressHash << " (" << _storage.size() << " values in storage)\n";
*/				storageDB.insert(hashAndValue.first, hashAndValue.second);
			}
			assert(storageDB.root());
			s.append(storageDB.root());
		}

		s << _codeHash;

		m_trie.insert(_addressHash, &s.out());
	}

	h256 importCode(bytesConstRef _code)
	{
		h256 const hash = sha3(_code);
		m_trie.db()->insert(hash, _code);
		return hash;
	}

	void commitStateDatabase() { m_trie.db()->commit(); }

	h256 stateRoot() const { return m_trie.root(); }

	std::string lookupCode(h256 const& _hash) const { return m_trie.db()->lookup(_hash);  }

	bool containsAccount(h256 const& _addressHash) const { return m_trie.contains(_addressHash); }

private:
	SpecificTrieDB<GenericTrieDB<OverlayDB>, h256> m_trie;
};

}
}
