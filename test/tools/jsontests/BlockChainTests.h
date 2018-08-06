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
/** @file BlockChainTests.h
 * BlockChainTests functions.
 */

#pragma once
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <test/tools/libtesteth/TestSuite.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <boost/filesystem/path.hpp>

using namespace dev;

namespace dev
{
namespace test
{

class BlockchainTestSuite: public TestSuite
{
public:
    json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

class BCGeneralStateTestsSuite: public BlockchainTestSuite
{
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

class TransitionTestsSuite: public TestSuite
{
    json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

struct ChainBranch
{
    ChainBranch(TestBlock const& _genesis, TestBlockChain::MiningType _miningType);
    void reset(TestBlockChain::MiningType _miningType);
    void restoreFromHistory(size_t _importBlockNumber);
    TestBlockChain blockchain;
    vector<TestBlock> importedBlocks;

    static void forceBlockchain(string const& chainname);
    static void resetBlockchain();

private:
    static eth::Network s_tempBlockchainNetwork;
};

//Functions that working with test json
void compareBlocks(TestBlock const& _a, TestBlock const& _b);
mArray writeTransactionsToJson(TransactionQueue const& _txsQueue);
mObject writeBlockHeaderToJson(BlockHeader const& _bi);
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, ChainBranch const& _chainBranch);
void overwriteUncleHeaderForTest(mObject& _uncleHeaderObj, TestBlock& _uncle, vector<TestBlock> const& _uncles, ChainBranch const& _chainBranch);
void eraseJsonSectionForInvalidBlock(mObject& _blObj);
void checkJsonSectionForInvalidBlock(mObject& _blObj);
void checkExpectedException(mObject& _blObj, Exception const& _e);
void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname);
bigint calculateMiningReward(u256 const& _blNumber, u256 const& _unNumber1, u256 const& _unNumber2, SealEngineFace const& _sealEngine);
json_spirit::mObject fillBCTest(json_spirit::mObject const& _input);
void testBCTest(json_spirit::mObject const& _o);

} } // Namespace Close
