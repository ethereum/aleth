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
/** @file StateTests.cpp
 * @author Dimitry Khokhlov <dimitry@ethereum.org>
 * @date 2016
 * General State Tests parser.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libethereum/BlockChain.h>
#include <libethereum/State.h>
#include <libethereum/ExtVM.h>
#include <libethereum/Defaults.h>
#include <libevm/VM.h>
#include <test/tools/libtesteth/TestHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

json_spirit::mValue doStateTests(json_spirit::mValue const& _input, bool _fillin)
{
	BOOST_REQUIRE_MESSAGE(_input.type() == obj_type,
		TestOutputHelper::testFileName() + " A GeneralStateTest file should contain an object.");
	BOOST_REQUIRE_MESSAGE(!_fillin || _input.get_obj().size() == 1,
		TestOutputHelper::testFileName() + " A GeneralStateTest filler should contain only one test.");
	json_spirit::mValue v = json_spirit::mObject();

	for (auto& i: _input.get_obj())
	{
		string const testname = i.first;
		BOOST_REQUIRE_MESSAGE(i.second.type() == obj_type,
			TestOutputHelper::testFileName() + " should contain an object under a test name.");
		json_spirit::mObject const& inputTest = i.second.get_obj();
		v.get_obj()[testname] = json_spirit::mObject();
		json_spirit::mObject& outputTest = v.get_obj()[testname].get_obj();

		if (_fillin && !TestOutputHelper::testFileName().empty())
			BOOST_REQUIRE_MESSAGE(testname + "Filler.json" == TestOutputHelper::testFileName(),
				TestOutputHelper::testFileName() + " contains a test with a different name '" + testname + "'" );

		if (!TestOutputHelper::passTest(testname))
			continue;

		//For 100% at the log output when making blockchain tests out of state tests
		if (_fillin == false && Options::get().fillchain)
			continue;

		BOOST_REQUIRE_MESSAGE(inputTest.count("env") > 0, testname + " env not set!");
		BOOST_REQUIRE_MESSAGE(inputTest.count("pre") > 0, testname + " pre not set!");
		BOOST_REQUIRE_MESSAGE(inputTest.count("transaction") > 0, testname + " transaction not set!");

		ImportTest importer(inputTest, outputTest);

		Listener::ExecTimeGuard guard{i.first};
		importer.executeTest();
		if (Options::get().fillchain)
			continue;

		if (_fillin)
		{
#if ETH_FATDB
			if (importer.exportTest())
				cerr << testname << endl;
#else
			BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(testname + " You can not fill tests when FATDB is switched off"));
#endif
		}
		else
		{
			BOOST_REQUIRE_MESSAGE(inputTest.count("post") > 0, testname + " post not set!");
			BOOST_REQUIRE_MESSAGE(inputTest.at("post").type() == obj_type, testname + " post field is not an object.");

			//check post hashes against cpp client on all networks
			mObject post = inputTest.at("post").get_obj();
			vector<size_t> wrongTransactionsIndexes;
			for (mObject::const_iterator i = post.begin(); i != post.end(); ++i)
			{
				BOOST_REQUIRE_MESSAGE(i->second.type() == array_type, testname + " post field should contain an array for each network.");
				for (auto const& exp: i->second.get_array())
				{
					BOOST_REQUIRE_MESSAGE(exp.type() == obj_type, " post field should contain an array of objects for each network.");
					if (!Options::get().singleTestNet.empty() && i->first != Options::get().singleTestNet)
						continue;
					if (test::isDisabledNetwork(test::stringToNetId(i->first)))
						continue;
					importer.checkGeneralTestSection(exp.get_obj(), wrongTransactionsIndexes, i->first);
				}
			}

			if (Options::get().statediff)
				importer.traceStateDiff();
		}
	}
	return v;
}
} }// Namespace Close

class generaltestfixture
{
public:
	generaltestfixture()
	{
		string casename = boost::unit_test::framework::current_test_case().p_name;
		if (casename == "stQuadraticComplexityTest" && !test::Options::get().all)
		{
			cnote << "Skipping " << casename << " because --all option is not specified.\n";
			return;
		}
		fillAllFilesInFolder(casename);
	}

	void fillAllFilesInFolder(string const& _folder)
	{
		std::string fillersPath = test::getTestPath() + "/src/GeneralStateTestsFiller/" + _folder;

		string filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
		std::vector<boost::filesystem::path> files = test::getJsonFiles(fillersPath, filter);
		int fileCount = files.size();

		if (test::Options::get().filltests)
			fileCount *= 2; //tests are checked when filled and after they been filled
		test::TestOutputHelper::initTest(fileCount);

		for (auto const& file: files)
		{
			test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
			test::executeTests(file.filename().string(), "/GeneralStateTests/"+_folder, "/GeneralStateTestsFiller/"+_folder, dev::test::doStateTests);
		}

		test::TestOutputHelper::finishTest();
	}
};

std::string const test::c_StateTestsGeneral = "StateTestsGeneral";
BOOST_FIXTURE_TEST_SUITE(StateTestsGeneral, generaltestfixture)

//Frontier Tests
BOOST_AUTO_TEST_CASE(stCallCodes){}
BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest){}
BOOST_AUTO_TEST_CASE(stExample){}
BOOST_AUTO_TEST_CASE(stInitCodeTest){}
BOOST_AUTO_TEST_CASE(stLogTests){}
BOOST_AUTO_TEST_CASE(stMemoryTest){}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts){}
BOOST_AUTO_TEST_CASE(stRandom){}
BOOST_AUTO_TEST_CASE(stRecursiveCreate){}
BOOST_AUTO_TEST_CASE(stRefundTest){}
BOOST_AUTO_TEST_CASE(stSolidityTest){}
BOOST_AUTO_TEST_CASE(stSpecialTest){}
BOOST_AUTO_TEST_CASE(stSystemOperationsTest){}
BOOST_AUTO_TEST_CASE(stTransactionTest){}
BOOST_AUTO_TEST_CASE(stTransitionTest){}
BOOST_AUTO_TEST_CASE(stWalletTest){}

//Homestead Tests
BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead){}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead){}
BOOST_AUTO_TEST_CASE(stHomesteadSpecific){}
BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead){}

//EIP150 Tests
BOOST_AUTO_TEST_CASE(stChangedEIP150){}
BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices){}
BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls){}
BOOST_AUTO_TEST_CASE(stEIP150Specific){}

//EIP158 Tests
BOOST_AUTO_TEST_CASE(stEIP158Specific){}
BOOST_AUTO_TEST_CASE(stNonZeroCallsTest){}
BOOST_AUTO_TEST_CASE(stZeroCallsTest){}
BOOST_AUTO_TEST_CASE(stZeroCallsRevert){}
BOOST_AUTO_TEST_CASE(stCodeSizeLimit){}
BOOST_AUTO_TEST_CASE(stCreateTest){}
BOOST_AUTO_TEST_CASE(stRevertTest){}

//Metropolis Tests
BOOST_AUTO_TEST_CASE(stStackTests){}
BOOST_AUTO_TEST_CASE(stStaticCall){}
BOOST_AUTO_TEST_CASE(stReturnDataTest){}
BOOST_AUTO_TEST_CASE(stZeroKnowledge){}

//Stress Tests
BOOST_AUTO_TEST_CASE(stAttackTest){}
BOOST_AUTO_TEST_CASE(stMemoryStressTest){}
BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest){}

//Invalid Opcode Tests
BOOST_AUTO_TEST_CASE(stBadOpcode){}
BOOST_AUTO_TEST_SUITE_END()
