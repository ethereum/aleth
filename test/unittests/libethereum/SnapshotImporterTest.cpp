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

#include <libethereum/SnapshotImporter.h>
#include <libethereum/StateImporter.h>
#include <libethereum/BlockChainImporter.h>
#include <libethereum/SnapshotStorage.h>
#include <test/tools/libtesteth/TestHelper.h>

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace
{
	struct ImportedAccount
	{
		h256 address;
		u256 nonce;
		u256 balance;
		std::map<h256, bytes> storage;
		h256 codeHash;
	};

	class MockStateImporter: public StateImporterFace
	{
	public:
		void importAccount(h256 const& _addressHash, u256 const& _nonce, u256 const& _balance, std::map<h256, bytes> const& _storage, h256 const& _codeHash) override
		{
			importedAccounts.push_back({_addressHash, _nonce, _balance, _storage,  _codeHash});
		}

		h256 importCode(bytesConstRef _code) override
		{ 
			importedCodes.push_back(_code.toBytes());
			return sha3(_code);
		}
		void commitStateDatabase() override { ++commitCounter; }
		h256 stateRoot() const override { return h256{}; }
		std::string lookupCode(h256 const& _hash) const override
		{ 
			auto it = std::find_if(importedCodes.begin(), importedCodes.end(), [&_hash](bytes const& _code) { return sha3(_code) == _hash; });
			return it == importedCodes.end() ? std::string{} : std::string(it->begin(), it->end());
		}

		std::vector<ImportedAccount> importedAccounts;
		std::vector<bytes> importedCodes;
		int commitCounter = 0;
	};

	class MockBlockChainImporter: public BlockChainImporterFace
	{
	public:
		void importBlock(BlockHeader const& /*_header*/, RLP /*_transactions*/, RLP /*_uncles*/, RLP /*_receipts*/, u256 const& /*_totalDifficulty*/) override {}
		void setChainStartBlockNumber(u256 const& /*_number*/) override {}
	};

	class MockSnapshotStorage: public SnapshotStorageFace
	{
	public:
		bytes readManifest() const override { return manifest; }
		std::string readChunk(h256 const& _chunkHash) const override
		{ 
			auto it = chunks.find(_chunkHash);
			return it == chunks.end() ? std::string{} : std::string(it->second.begin(), it->second.end());
		}

		bytes manifest;
		std::map<h256, bytes> chunks;
	};

	class SnapshotImporterTestFixture: public TestOutputHelper
	{
	public:
		SnapshotImporterTestFixture(): snapshotImporter(stateImporter, blockChainImporter) {}

		MockStateImporter stateImporter;
		MockBlockChainImporter blockChainImporter;
		MockSnapshotStorage snapshotStorage;
		SnapshotImporter snapshotImporter;
	};

	bytes createManifest(int _version, h256s const& _stateChunks, h256s const& _blockChunks, h256 const& _stateRoot, unsigned _blockNumber, h256 const& _blockHash)
	{
		RLPStream s(6);
		s << _version;
		s.appendList(_stateChunks.size());
		for (auto chunk: _stateChunks)
			s << chunk;
		s.appendList(_blockChunks.size());
		for (auto chunk: _blockChunks)
			s << chunk;
		s << _stateRoot << _blockNumber << _blockHash;
		return s.out();
	}

	bytes createAccount(u256 const& _nonce, u256 const& _balance, byte _codeFlag, bytes const& _code, std::map<h256, bytes> _storage)
	{
		RLPStream s(5);
		s << _nonce << _balance << _codeFlag << _code;
		s.appendList(_storage.size());
		for (auto& keyValue: _storage)
		{
			s.appendList(2);
			s << keyValue.first << keyValue.second;
		}
			
		return s.out();
	}

	bytes createStateChunk(std::vector<std::pair<h256, bytes>> const& _accounts)
	{
		RLPStream s(_accounts.size());
		for (auto& addressAccount: _accounts)
		{
			s.appendList(2);
			s << addressAccount.first;
			s.appendRaw(addressAccount.second);
		}

		return s.out();
	}
}

BOOST_FIXTURE_TEST_SUITE(SnapshotImporterSuite, SnapshotImporterTestFixture)

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_importChecksManifestVersion)
{
	RLPStream s(6);
	s << 3;
	s.appendList(0);
	s.appendList(0);
	s << h256{} << 0 << h256{};
	snapshotStorage.manifest = createManifest(3, {}, {}, h256{}, 0, h256{});

	BOOST_REQUIRE_THROW(snapshotImporter.import(snapshotStorage), UnsupportedSnapshotManifestVersion);
}

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_importNonsplittedAccount)
{
	h256 stateChunk = sha3("123");
	snapshotStorage.manifest = createManifest(2, {stateChunk}, {}, h256{}, 0, h256{});

	bytes account = createAccount(1, 10, 0, {0x80}, {});
	h256 addressHash = sha3("456");
	bytes chunkBytes = createStateChunk({{addressHash, account}});
	snapshotStorage.chunks[stateChunk] = chunkBytes;
	
	snapshotImporter.import(snapshotStorage);

	BOOST_REQUIRE_EQUAL(stateImporter.importedAccounts.size(), 1);
	ImportedAccount const& importedAccount = stateImporter.importedAccounts.front();
	BOOST_CHECK_EQUAL(importedAccount.address, addressHash);
	BOOST_CHECK_EQUAL(importedAccount.nonce, 1);
	BOOST_CHECK_EQUAL(importedAccount.balance, 10);
	BOOST_CHECK_EQUAL(importedAccount.codeHash, EmptySHA3);
	BOOST_CHECK(importedAccount.storage.empty());
}

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_importSplittedAccount)
{
	h256 stateChunk1 = sha3("123");
	h256 stateChunk2 = sha3("789");
	snapshotStorage.manifest = createManifest(2, {stateChunk1, stateChunk2}, {}, h256{}, 0, h256{});

	h256 addressHash = sha3("456");
	std::pair<h256, bytes> storagePair1{sha3("111"), {1}};
	std::pair<h256, bytes> storagePair2{sha3("222"), {2}};
	bytes accountPart1 = createAccount(2, 10, 0, {0x80}, {storagePair1, storagePair2});
	bytes chunk1 = createStateChunk({{addressHash, accountPart1}});
	snapshotStorage.chunks[stateChunk1] = chunk1;

	std::pair<h256, bytes> storagePair3{sha3("333"),{3}};
	std::pair<h256, bytes> storagePair4{sha3("444"),{4}};
	bytes accountPart2 = createAccount(2, 10, 0, {0x80}, {storagePair3, storagePair4});
	bytes chunk2 = createStateChunk({{addressHash, accountPart2}});
	snapshotStorage.chunks[stateChunk2] = chunk2;

	snapshotImporter.import(snapshotStorage);

	BOOST_REQUIRE_EQUAL(stateImporter.importedAccounts.size(), 1);
	ImportedAccount const& importedAccount = stateImporter.importedAccounts.front();
	BOOST_CHECK_EQUAL(importedAccount.address, addressHash);
	BOOST_CHECK_EQUAL(importedAccount.nonce, 2);
	BOOST_CHECK_EQUAL(importedAccount.balance, 10);
	BOOST_CHECK_EQUAL(importedAccount.codeHash, EmptySHA3);

	std::map<h256, bytes> expectedStorage{storagePair1, storagePair2, storagePair3, storagePair4};
	BOOST_CHECK_EQUAL(importedAccount.storage, expectedStorage);
}

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_importAccountWithCode)
{
	h256 stateChunk = sha3("123");
	snapshotStorage.manifest = createManifest(2, {stateChunk}, {}, h256{}, 0, h256{});

	bytes code = {1, 2, 3};
	bytes account = createAccount(1, 10, 1, code, {});
	h256 addressHash = sha3("456");
	bytes chunkBytes = createStateChunk({{addressHash, account}});
	snapshotStorage.chunks[stateChunk] = chunkBytes;

	snapshotImporter.import(snapshotStorage);

	BOOST_REQUIRE_EQUAL(stateImporter.importedAccounts.size(), 1);
	ImportedAccount const& importedAccount = stateImporter.importedAccounts.front();
	BOOST_CHECK_EQUAL(importedAccount.address, addressHash);
	BOOST_CHECK_EQUAL(importedAccount.codeHash, sha3(code));

	BOOST_REQUIRE_EQUAL(stateImporter.importedCodes.size(), 1);
	bytes const& importedCode = stateImporter.importedCodes.front();
	BOOST_CHECK_EQUAL_COLLECTIONS(importedCode.begin(), importedCode.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_importAccountsWithEqualCode)
{
	h256 stateChunk = sha3("123");
	snapshotStorage.manifest = createManifest(2, {stateChunk}, {}, h256{}, 0, h256{});

	bytes code = {1, 2, 3};
	bytes account1 = createAccount(1, 10, 1, code, {});
	h256 addressHash1 = sha3("456");

	h256 codeHash = sha3(code);
	bytes account2 = createAccount(1, 10, 2, codeHash.asBytes(), {});
	h256 addressHash2 = sha3("789");

	snapshotStorage.chunks[stateChunk] = createStateChunk({{addressHash1, account1}, {addressHash2, account2}});

	snapshotImporter.import(snapshotStorage);

	BOOST_REQUIRE_EQUAL(stateImporter.importedAccounts.size(), 2);
	BOOST_CHECK_EQUAL(stateImporter.importedAccounts[0].address, addressHash1);
	BOOST_CHECK_EQUAL(stateImporter.importedAccounts[0].codeHash, codeHash);
	BOOST_CHECK_EQUAL(stateImporter.importedAccounts[1].address, addressHash2);
	BOOST_CHECK_EQUAL(stateImporter.importedAccounts[1].codeHash, codeHash);

	BOOST_REQUIRE_EQUAL(stateImporter.importedCodes.size(), 1);
	bytes const& importedCode = stateImporter.importedCodes.front();
	BOOST_CHECK_EQUAL_COLLECTIONS(importedCode.begin(), importedCode.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(SnapshotImporterSuite_commitStateOnceEveryChunk)
{
	h256 stateChunk1 = sha3("123");
	h256 stateChunk2 = sha3("789");
	snapshotStorage.manifest = createManifest(2, {stateChunk1, stateChunk2}, {}, h256{}, 0, h256{});

	h256 addressHash1 = sha3("456");
	bytes accountPart1 = createAccount(2, 10, 0, {0x80}, {});
	bytes chunk1 = createStateChunk({{addressHash1, accountPart1}});
	snapshotStorage.chunks[stateChunk1] = chunk1;

	h256 addressHash2 = sha3("345");
	bytes accountPart2 = createAccount(2, 10, 0, {0x80}, {});
	bytes chunk2 = createStateChunk({{addressHash2, accountPart2}});
	snapshotStorage.chunks[stateChunk2] = chunk2;

	snapshotImporter.import(snapshotStorage);

	BOOST_REQUIRE_EQUAL(stateImporter.importedAccounts.size(), 2);
	BOOST_REQUIRE_EQUAL(stateImporter.commitCounter, 3); // once every chunk + 1 last commit
}

BOOST_AUTO_TEST_SUITE_END()
