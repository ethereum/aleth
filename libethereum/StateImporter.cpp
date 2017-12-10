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

#include "StateImporter.h"

#include <libdevcore/OverlayDB.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>

namespace dev
{
namespace eth
{

namespace
{

class StateImporter: public StateImporterFace
{
public:
	explicit StateImporter(OverlayDB& _stateDb): m_trie(&_stateDb) { m_trie.init(); }

	void importAccount(h256 const& _addressHash, u256 const& _nonce, u256 const& _balance, std::map<h256, bytes> const& _storage, h256 const& _codeHash) override
	{
		RLPStream s(4);
		s << _nonce << _balance;

		h256 const storageRoot = isAccountImported(_addressHash) ? accountStorageRoot(_addressHash) : EmptyTrie;
		if (_storage.empty())
			s.append(storageRoot);
		else
		{
			SpecificTrieDB<GenericTrieDB<OverlayDB>, h256> storageDB(m_trie.db(), storageRoot);
			for (auto const& hashAndValue: _storage)
				storageDB.insert(hashAndValue.first, hashAndValue.second);

			s.append(storageDB.root());
		}

		s << _codeHash;

		m_trie.insert(_addressHash, &s.out());
	}

	h256 importCode(bytesConstRef _code) override
	{
		h256 const hash = sha3(_code);
		m_trie.db()->insert(hash, _code);
		return hash;
	}

	void commitStateDatabase() override { m_trie.db()->commit(); }

	bool isAccountImported(h256 const& _addressHash) const override { return m_trie.contains(_addressHash); }

	h256 stateRoot() const override { return m_trie.root(); }

	std::string lookupCode(h256 const& _hash) const override { return m_trie.db()->lookup(_hash); }

private:
	// can be used only with already imported accounts
	h256 accountStorageRoot(h256 const& _addressHash) const
	{
		std::string const account = m_trie.at(_addressHash);
		assert(!account.empty());
		
		RLP accountRlp(account);
		if (accountRlp.itemCount() < 3)
			BOOST_THROW_EXCEPTION(InvalidAccountInTheDatabase());

		return accountRlp[2].toHash<h256>(RLP::VeryStrict);
	}

	SpecificTrieDB<GenericTrieDB<OverlayDB>, h256> m_trie;
};

}

std::unique_ptr<StateImporterFace> createStateImporter(OverlayDB& _stateDb)
{
	return std::unique_ptr<StateImporterFace>(new StateImporter(_stateDb));
}

}
}
