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
 */

#pragma once
#include "JsonSpiritHeaders.h"
#include <libethereum/BlockChain.h>
#include <libethereum/TransactionQueue.h>
#include <libdevcore/TransientDirectory.h>
#include <libethashseal/GenesisInfo.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;


namespace dev {  namespace test {

struct BlockStateUndefined : virtual Exception {};
class TestTransaction;
class TestBlock;
class TestBlockChain;

class TestTransaction
{
public:
	TestTransaction(json_spirit::mObject const& _o);
	TestTransaction(Transaction const& _tr) : m_transaction(_tr) {}
	Transaction const& transaction() const { return m_transaction; }
	json_spirit::mObject& jsonObject() { return m_jsonTransaction; }
	static TestTransaction defaultTransaction(u256 const& _nonce = 1, u256 const& _gasPrice = 1, u256 const& _gasLimit = 50000, bytes const& _data = bytes());

private:
	json_spirit::mObject m_jsonTransaction;
	Transaction m_transaction;
};

class TestBlock
{
public:
	TestBlock();
	TestBlock(std::string const& _blockRlp);
	TestBlock(mObject const& _blockObj, mObject const& _stateObj);
	TestBlock(TestBlock const& _original);
	TestBlock& operator=(TestBlock const& _original);

	void addTransaction(TestTransaction const& _tr);
	void addUncle(TestBlock const& _uncle);
	void setUncles(vector<TestBlock> const& _uncles);
	void setPremine(std::string const& _parameter) { m_premineUpdate[_parameter] = true; }
	void noteDirty() { m_dirty = true; }
	void mine(TestBlockChain const& _bc);
	void updateNonce(TestBlockChain const& _bc);
	void verify(TestBlockChain const& _bc) const;

	void setBlockHeader(BlockHeader const& _header);
	void setState(State const& _state);
	void clearState();

	BlockHeader const& premineHeader() { return m_premineHeader; } //should return fields according to m_premineUpdate. this is needed to check that premine chanes was not lost during mining .
	dev::bytes const& bytes() const { return m_bytes; }
	bytesConstRef receipts() const { return bytesConstRef(&m_receipts.out()[0], m_receipts.out().size()); }
	AccountMap const& accountMap() const { return m_accountMap; }
	State const& state() const { if (m_state.get() == 0) BOOST_THROW_EXCEPTION(BlockStateUndefined() << errinfo_comment("Block State is Nulled")); return *m_state.get(); }
	BlockHeader const& blockHeader() const { return m_blockHeader;}
	TransactionQueue const& transactionQueue() const { return m_transactionQueue; }
	TransactionQueue & transactionQueue() { return m_transactionQueue; }
	vector<TestTransaction> const& testTransactions() const { return m_testTransactions; }
	vector<TestBlock> const& uncles() const { return m_uncles; }
	Address const& beneficiary() const { return m_blockHeader.author(); }

private:
	BlockHeader constructBlock(mObject const& _o, h256 const& _stateRoot);
	dev::bytes createBlockRLPFromFields(mObject const& _tObj, h256 const& _stateRoot = h256{});
	void recalcBlockHeaderBytes();
	void copyStateFrom(State const& _state);
	void populateFrom(TestBlock const& _original);
	void premineUpdate(BlockHeader& info);

	bool m_dirty;
	BlockHeader m_blockHeader;
	vector<TestBlock> m_uncles;
	std::unique_ptr<State> m_state;
	TransactionQueue m_transactionQueue;
	BlockQueue m_uncleQueue;
	dev::bytes m_bytes;
	std::unique_ptr<TransientDirectory> m_tempDirState;
	vector<TestTransaction> m_testTransactions;
	std::map<std::string, bool> m_premineUpdate;			//Test Header alterate options. TODO: Do we really need this?
	BlockHeader m_premineHeader;
	AccountMap m_accountMap;								//Needed for genesis state
	RLPStream m_receipts;
};

class TestBlockChain
{
public:
	TestBlockChain(): TestBlockChain(defaultGenesisBlock()) {}
	TestBlockChain(TestBlock const& _genesisBlock);

	void reset(TestBlock const& _genesisBlock);
	bool addBlock(TestBlock const& _block);
	vector<TestBlock> syncUncles(vector<TestBlock> const& _uncles);
	TestBlock const& topBlock() { return m_lastBlock; }
	BlockChain const& interface() const { return *m_blockChain;}
	BlockChain& interfaceUnsafe() const { return *m_blockChain;}
	TestBlock const& testGenesis() const { return m_genesisBlock; }

	static json_spirit::mObject defaultGenesisBlockJson();
	static TestBlock defaultGenesisBlock(u256 const& _gasLimit = DefaultBlockGasLimit);
	static AccountMap defaultAccountMap();
	static eth::Network s_sealEngineNetwork;
private:

	std::unique_ptr<BlockChain> m_blockChain;
	TestBlock m_genesisBlock;
	TestBlock m_lastBlock;
	std::unique_ptr<TransientDirectory> m_tempDirBlockchain;
};

}}
