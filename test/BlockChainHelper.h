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
/** @file BlockChainHelper.h
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 */

#include <libethereum/BlockChain.h>
#include <libethereum/TransactionQueue.h>
#include <libdevcore/TransientDirectory.h>

#include <test/TestHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

class TestTransaction;
class TestBlock;
class TestBlockChain;

class TestTransaction
{
public:
	TestTransaction(json_spirit::mObject const& _o);
	Transaction const& getTransaction() const { return m_transaction; }
private:
	json_spirit::mObject m_jsonTransaction;
	Transaction  m_transaction;
};

class TestBlock
{
private:
	typedef Ethash::BlockHeader BlockHeader;

public:
	TestBlock() {};
	TestBlock(TestBlock const& _original);
	TestBlock& operator = (TestBlock const& _original);
	TestBlock(json_spirit::mObject& _blockObj, json_spirit::mObject& _stateObj);
	void addTransaction(TestTransaction const& _tr);
	void mine(TestBlockChain const& bc);
	void overwriteBlockHeader(mObject const& _blObj, const BlockHeader& _parent);

	bytes const& getBytes() const { return m_bytes; }
	State const& getState() const { return m_state; }
	BlockHeader const& getBlockHeader() const { return m_blockHeader;}
	TransactionQueue const& getTransactionQueue() const { return m_transactionQueue; }
	TransactionQueue & getTransactionQueue() { return m_transactionQueue; }
	vector<TestTransaction> const& getTestTransactions() const { return m_testTransactions; }
	Address const& getBeneficiary() const { return m_blockHeader.beneficiary(); }

private:
	BlockHeader constructBlock(mObject& _o, h256 const& _stateRoot);
	bytes createBlockRLPFromFields(mObject& _tObj, h256 const& _stateRoot = h256{});
	void recalcBlockHeaderBytes();
	void copyStateFrom(State const& _state);
	void populateFrom(TestBlock const& _original);

private:
	BlockHeader m_blockHeader;
	State m_state;
	TransactionQueue m_transactionQueue;
	bytes m_bytes;
	TransientDirectory m_tempDirState;
	vector<TestTransaction> m_testTransactions;
};

class TestBlockChain
{
private:
	typedef FullBlockChain<Ethash> FullBlockChainEthash;
public:
	TestBlockChain(TestBlock const& _genesisBlock);
	void addBlock(TestBlock& _block);
	FullBlockChain<Ethash> const& getInterface() const { return *m_blockChain.get();}
	TestBlock const& getTestGenesis() const { return m_genesisBlock; }
private:
	std::unique_ptr<FullBlockChainEthash> m_blockChain;
	TestBlock m_genesisBlock;
	TransientDirectory m_tempDirBlockchain;
};

}}
