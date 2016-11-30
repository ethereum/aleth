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
 * State test functions.
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
#include <test/TestHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

void doStateTests2(json_spirit::mValue& _v, bool _fillin)
{
	for (auto& i: _v.get_obj())
	{
		string testname = i.first;
		json_spirit::mObject& o = i.second.get_obj();

		if (!TestOutputHelper::passTest(o, testname))
			continue;

		BOOST_REQUIRE_MESSAGE(o.count("env") > 0, testname + "env not set!");
		BOOST_REQUIRE_MESSAGE(o.count("pre") > 0, testname + "pre not set!");
		BOOST_REQUIRE_MESSAGE(o.count("transaction") > 0, testname + "transaction not set!");

		ImportTest importer(o, _fillin, testType::GeneralStateTest);
		const State importedStatePost = importer.m_statePost;

		Listener::ExecTimeGuard guard{i.first};
		importer.executeTest();

		if (_fillin)
		{
#if ETH_FATDB
			if (importer.exportTest(bytes()))
				cerr << testname << endl;
#else
			BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(testname + "You can not fill tests when FATDB is switched off"));
#endif
		}
		else
		{
			BOOST_REQUIRE(o.count("post") > 0);

			//check post hashes against cpp client on all networks
			mObject post = o["post"].get_obj();
			vector<size_t> wrongTransactionsIndexes;
			for (mObject::const_iterator i = post.begin(); i != post.end(); ++i)
			{
				for (auto const& exp: i->second.get_array())
					importer.checkGeneralTestSection(exp.get_obj(), wrongTransactionsIndexes, i->first);
			}
		}
	}
}
} }// Namespace Close

class generaltestfixture
{
public:
	generaltestfixture()
	{
		string casename = boost::unit_test::framework::current_test_case().p_name;
		if (casename == "stMemoryStressTest" && (!test::Options::get().memory || 1))
			return;
		if (casename == "stQuadraticComplexityTest" && !test::Options::get().quadratic)
			return;
		fillAllFilesInFolder(casename);
	}

	void fillAllFilesInFolder(string _folder)
	{
		std::string fillersPath = dev::test::getFolder(__FILE__) + "/GeneralStateTestsFiller/" + _folder;

		boost::filesystem::directory_iterator iterator_tmp(fillersPath);
		int fileCount = 0;
		for(; iterator_tmp != boost::filesystem::directory_iterator(); ++iterator_tmp)
			if (boost::filesystem::is_regular_file(iterator_tmp->path()) && iterator_tmp->path().extension() == ".json")
				fileCount++;
		if (dev::test::Options::get().fillTests)
			fileCount *= 2; //tests are checked when filled and after they been filled
		dev::test::TestOutputHelper::initTest(fileCount);

		boost::filesystem::directory_iterator iterator(fillersPath);
		for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
			if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
			{
				string fileboost = iterator->path().filename().string();
				dev::test::executeTests(fileboost, "/GeneralStateTests/"+_folder,dev::test::getFolder(__FILE__) + "/GeneralStateTestsFiller/"+_folder, dev::test::doStateTests2);
			}
	}
};

BOOST_FIXTURE_TEST_SUITE(StateTestsGeneral, generaltestfixture)

BOOST_AUTO_TEST_CASE(stHomesteadSpecific){}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead){}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead){}
BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead){}

BOOST_AUTO_TEST_CASE(stRecursiveCreate){}
BOOST_AUTO_TEST_CASE(stBlockHashTest){}
BOOST_AUTO_TEST_CASE(stCallCodes){}
BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest){}
BOOST_AUTO_TEST_CASE(stExample){}
BOOST_AUTO_TEST_CASE(stInitCodeTest){}
BOOST_AUTO_TEST_CASE(stLogTests){}
BOOST_AUTO_TEST_CASE(stMemoryStressTest){}
BOOST_AUTO_TEST_CASE(stMemoryTest){}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts){}
BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest){}
BOOST_AUTO_TEST_CASE(stRandom){}
BOOST_AUTO_TEST_CASE(stRefundTest){}
BOOST_AUTO_TEST_CASE(stSolidityTest){}
BOOST_AUTO_TEST_CASE(stSpecialTest){}
BOOST_AUTO_TEST_CASE(stSystemOperationsTest){}
BOOST_AUTO_TEST_CASE(stTransactionTest){}
BOOST_AUTO_TEST_CASE(stWalletTest){}
BOOST_AUTO_TEST_SUITE_END()
