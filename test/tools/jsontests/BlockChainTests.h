// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2017-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <test/tools/jsontests/StateTestFixtureBase.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/TestSuite.h>
#include <boost/filesystem/path.hpp>

using namespace dev;

namespace dev
{
namespace test
{
class BlockchainInvalidTestSuite : public TestSuite
{
public:
    json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

class BlockchainValidTestSuite : public TestSuite
{
public:
    json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

class BCGeneralStateTestsSuite : public BlockchainValidTestSuite
{
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};

class bcGeneralTestsFixture : public StateTestFixtureBase<BCGeneralStateTestsSuite>
{
public:
    bcGeneralTestsFixture()
      : StateTestFixtureBase({TestExecution::RequireOptionAll, TestExecution::NotRefillable})
    {}
};

template <class T>
class bcTestFixture
{
public:
    bcTestFixture(std::set<TestExecution> const& _execFlags)
    {
        T suite;
        if (_execFlags.count(TestExecution::NotRefillable) &&
            (Options::get().fillchain || Options::get().filltests))
            BOOST_FAIL("Tests are sealed and not refillable!");

        string const casename = boost::unit_test::framework::current_test_case().p_name;
        boost::filesystem::path const suiteFillerPath = suite.getFullPathFiller(casename).parent_path();

        // skip wallet test as it takes too much time (250 blocks) run it with --all flag
        if (casename == "bcWalletTest" && !test::Options::get().all)
        {
            cnote << "Skipping " << casename << " because --all option is not specified.\n";
            test::TestOutputHelper::get().markTestFolderAsFinished(suiteFillerPath, casename);
            return;
        }

        suite.runAllTestsInFolder(casename);
        test::TestOutputHelper::get().markTestFolderAsFinished(suiteFillerPath, casename);
    }
};

template <class T>
class bcTestFixtureNotRefillable : public bcTestFixture<T>
{
public:
    bcTestFixtureNotRefillable() : bcTestFixture<T>({TestExecution::NotRefillable}) {}
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
json_spirit::mObject fillBCTest(
    json_spirit::mObject const& _input, bool _allowInvalidBlocks = false);
void testBCTest(json_spirit::mObject const& _o);

} } // Namespace Close
