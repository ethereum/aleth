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
/** @file transactionTests.cpp
 * @author Dmitrii Khokhlov <winsvega@mail.ru>
 * @date 2015
 * Transaction test functions.
 */

#include <libethcore/SealEngine.h>
#include <libethashseal/GenesisInfo.h>
#include <libethereum/ChainParams.h>
#include <test/tools/libtestutils/Common.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
#include <test/tools/libtesteth/TestSuite.h>
#include <test/tools/libtesteth/ImportTest.h>
#include <test/tools/jsontests/TransactionTests.h>
#include <boost/filesystem/path.hpp>
#include <string>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

namespace dev {  namespace test {

mObject getExpectSection(mValue const& _expect, eth::Network _network)
{
    std::vector<mObject> objVector;
    BOOST_REQUIRE(_expect.type() == json_spirit::array_type);
    for (auto const& value: _expect.get_array())
    {
        BOOST_REQUIRE(value.type() == json_spirit::obj_type);
        mObject obj;
        obj = value.get_obj();
        BOOST_REQUIRE_MESSAGE(obj.count("network"), "network section not set in expect section!");
        set<string> networks;
        ImportTest::parseJsonStrValueIntoSet(obj["network"], networks);
        BOOST_CHECK_MESSAGE(networks.size() > 0, TestOutputHelper::get().testName() + " Network array not set!");
        ImportTest::checkAllowedNetwork(networks);

        if (networks.count(test::netIdToString(_network)) || networks.count(string("ALL")))
            objVector.push_back(obj);
    }
    BOOST_REQUIRE_MESSAGE(objVector.size() == 1, "Expect network should occur once in expect section of transaction test filler! (" + test::netIdToString(_network) + ")");
    return objVector.at(0);
}

json_spirit::mObject FillTransactionTest(json_spirit::mObject const& _o)
{
    mObject out;
    BOOST_REQUIRE_MESSAGE(_o.count("transaction") > 0, "transaction section not found! " + TestOutputHelper::get().testFile().string());
    BOOST_REQUIRE_MESSAGE(_o.count("expect") > 0, "expect section not found! " + TestOutputHelper::get().testFile().string());

    mObject tObj = _o.at("transaction").get_obj();
    RLPStream rlpStream = createRLPStreamFromTransactionFields(tObj);
    out["rlp"] = toHexPrefixed(rlpStream.out());

    // Theoretical block for transaction check
    BlockHeader bh;
    bh.setNumber(1);	//Seal engine below enables network rules from block 0
    bh.setGasLimit(u256("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

    mValue expectObj = _o.at("expect");
    for (auto const network: test::getNetworks())
    {
        ChainParams params(genesisInfo(network));
        unique_ptr<SealEngineFace> se(params.createSealEngine());

        // Test networks has forkblocks set to 0 if rules are enabled
        bool onExperimental = (bh.number() >= params.constantinopleForkBlock);

        out[test::netIdToString(network)] = mObject();
        mObject expectSection = getExpectSection(expectObj, network);
        try
        {
            Transaction txFromFields(rlpStream.out(), CheckTransaction::Everything);
            bool onExperimentalAndZeroSig = onExperimental && txFromFields.hasZeroSignature();
            if (!(txFromFields.signature().isValid() || onExperimentalAndZeroSig))
                BOOST_THROW_EXCEPTION(
                    Exception() << errinfo_comment(TestOutputHelper::get().testName() +
                                                   "transaction from RLP signature is invalid"));

            se->verifyTransaction(ImportRequirements::Everything, txFromFields, bh, 0);
            if (expectSection.count("sender") > 0)
            {
                string expectSender = toString(expectSection["sender"].get_str());
                BOOST_CHECK_MESSAGE(toString(txFromFields.sender()) == expectSender, "Error filling transaction test " + TestOutputHelper::get().testName() + ": expected another sender address! (got: " + toString(txFromFields.sender()) + "), expected: (" + expectSender + ")");
            }

            mObject resultObject;
            resultObject["sender"] = toString(txFromFields.sender());
            resultObject["hash"] = toString(txFromFields.sha3());
            out[test::netIdToString(network)] = resultObject;
        }
        catch (Exception const& _e)
        {
            //Transaction is InValid
            cnote << "Transaction Exception: " << diagnostic_information(_e);
            bool expectInValid = (expectSection["result"].get_str() == "invalid");
            BOOST_CHECK_MESSAGE(expectInValid, TestOutputHelper::get().testName() + " Check state: Transaction is expected to be valid! (" + test::netIdToString(network) + ")");
            continue;
        }

        //Transaction is Valid
        bool expectValid = (expectSection["result"].get_str() == "valid");
        BOOST_CHECK_MESSAGE(expectValid, TestOutputHelper::get().testName() + " Check state: Transaction is expected to be invalid! (" + test::netIdToString(network) + ")");
    }
    return out;
}

void TestTransactionTest(json_spirit::mObject const& _o)
{
    BOOST_REQUIRE(_o.count("rlp") > 0);
    string const& testname = TestOutputHelper::get().testName();

    // Theoretical block for transaction check
    BlockHeader bh;
    bh.setNumber(1);	//Seal engine below enables network rules from block 0
    bh.setGasLimit(u256("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

    for (auto const network: test::getNetworks())
    {
        Transaction txFromRlp;
        string networkname = test::netIdToString(network);
        BOOST_REQUIRE_MESSAGE(_o.count(networkname) > 0, testname + " Transaction test missing network results! (" + networkname + ")");
        BOOST_REQUIRE(_o.at(networkname).type() == json_spirit::obj_type);
        ChainParams params(genesisInfo(network));
        unique_ptr<SealEngineFace> se(params.createSealEngine());
        bool onExperimental = (bh.number() >= params.experimentalForkBlock);
        mObject obj = _o.at(networkname).get_obj();
        try
        {
            bytes stream = importByteArray(_o.at("rlp").get_str());
            RLP rlp(stream);
            txFromRlp = Transaction(rlp.data(), CheckTransaction::Everything);
            bool onExperimentalAndZeroSig = onExperimental && txFromRlp.hasZeroSignature();
            se->verifyTransaction(ImportRequirements::Everything, txFromRlp, bh, 0);
            if (!(txFromRlp.signature().isValid() || onExperimentalAndZeroSig))
                BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(
                                          testname + "transaction from RLP signature is invalid (" +
                                          networkname + ")"));
        }
        catch (Exception const& _e)
        {
            cnote << testname;
            cnote << "Transaction Exception: " << diagnostic_information(_e);
            BOOST_CHECK_MESSAGE(obj.count("hash") == 0,
                testname + "A transaction object should not be defined because the RLP is invalid! (" + networkname + ")");
            continue;
        }

        BOOST_REQUIRE(obj.count("sender") > 0);
        Address addressExpected = Address(obj["sender"].get_str());
        BOOST_CHECK_MESSAGE(txFromRlp.sender() == addressExpected, testname + "Signature address of sender does not match given sender address! (" + networkname + ")");

        BOOST_REQUIRE_MESSAGE(obj.count("hash") > 0, testname + "Expected a valid transaction! (" + networkname + ")");
        h256 txSha3Expected = h256(obj["hash"].get_str());
        BOOST_CHECK_MESSAGE(txFromRlp.sha3() == txSha3Expected, testname + "Expected different transaction hash! (" + networkname + ")");
    }
}

json_spirit::mValue TransactionTestSuite::doTests(json_spirit::mValue const& _input, bool _fillin) const
{
    BOOST_REQUIRE_MESSAGE(_input.type() == obj_type,
        TestOutputHelper::get().get().testFile().string() + " TransactionTest file should contain an object.");
    BOOST_REQUIRE_MESSAGE(!_fillin || _input.get_obj().size() == 1,
        TestOutputHelper::get().testFile().string() + " TransactionTest filler should contain only one test.");

    json_spirit::mObject v;
    for (auto const& i: _input.get_obj())
    {
        string const& testname = i.first;
        json_spirit::mObject const& o = i.second.get_obj();

        if (!TestOutputHelper::get().checkTest(testname))
            continue;

        if (_fillin)
            v[testname] = FillTransactionTest(o);
        else
            TestTransactionTest(o);
    }

    return v;
}

fs::path TransactionTestSuite::suiteFolder() const
{
    return "TransactionTests";
}

fs::path TransactionTestSuite::suiteFillerFolder() const
{
    return "TransactionTestsFiller";
}

} }// Namespace Close

class TransactionTestFixture
{
public:
    TransactionTestFixture()
    {
        string const& casename = boost::unit_test::framework::current_test_case().p_name;
        test::TransactionTestSuite suite;
        suite.runAllTestsInFolder(casename);
    }
};

BOOST_FIXTURE_TEST_SUITE(TransactionTests, TransactionTestFixture)

BOOST_AUTO_TEST_CASE(ttAddress){}
BOOST_AUTO_TEST_CASE(ttData){}
BOOST_AUTO_TEST_CASE(ttGasLimit){}
BOOST_AUTO_TEST_CASE(ttGasPrice){}
BOOST_AUTO_TEST_CASE(ttNonce){}
BOOST_AUTO_TEST_CASE(ttRSValue){}
BOOST_AUTO_TEST_CASE(ttValue){}
BOOST_AUTO_TEST_CASE(ttVValue){}
BOOST_AUTO_TEST_CASE(ttSignature){}
BOOST_AUTO_TEST_CASE(ttWrongRLP){}

BOOST_AUTO_TEST_SUITE_END()
