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
/** @file difficulty.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * difficulty calculation tests.
 */


#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>
#include <test/fuzzTesting/fuzzHelper.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethereum/ChainParams.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

namespace js = json_spirit;
std::string const c_testDifficulty = R"(
 "DifficultyTest[N]" : {
		"parentTimestamp" : "[PSTAMP]",
		"parentDifficulty" : "[PDIFF]",
		"currentTimestamp" : "[СSTAMP]",
		"currentBlockNumber" : "[CNUM]",
		"currentDifficulty" : "[CDIFF]"
	},
)";

void fillDifficulty(string const& _testFileFullName, Ethash& _sealEngine)
{
	int testN = 0;
	ostringstream finalTest;
	finalTest << "{" << std::endl;
	for (int stampDelta = 0; stampDelta < 15; stampDelta++)
	{
		for (u256 blockNumber = 10000; blockNumber < 1000000; blockNumber += 25000)
		{
			testN++;
			u256 pStamp = dev::test::RandomCode::randomUniInt();
			u256 pDiff = dev::test::RandomCode::randomUniInt();
			u256 cStamp = pStamp + stampDelta;
			u256 cNum = blockNumber;

			BlockHeader parent;
			parent.setTimestamp(pStamp);
			parent.setDifficulty(pDiff);
			parent.setNumber(cNum - 1);

			BlockHeader current;
			current.setTimestamp(cStamp);
			current.setNumber(cNum);

			string tmptest = c_testDifficulty;
			std::map<string, string> replaceMap;
			replaceMap["[N]"] = toString(testN);
			replaceMap["[PDIFF]"] = toCompactHex(pDiff, HexPrefix::Add);
			replaceMap["[PSTAMP]"] = toCompactHex(pStamp, HexPrefix::Add);
			replaceMap["[СSTAMP]"] = toCompactHex(cStamp, HexPrefix::Add);
			replaceMap["[CNUM]"] = toCompactHex(cNum, HexPrefix::Add);
			replaceMap["[CDIFF]"] = toCompactHex(_sealEngine.calculateDifficulty(current, parent), HexPrefix::Add);

			dev::test::RandomCode::parseTestWithTypes(tmptest, replaceMap);
			finalTest << tmptest;
		}
	}

	finalTest << std::endl << "}";
	writeFile(_testFileFullName, asBytes(finalTest.str()));
}

void testDifficulty(string const& _testFileFullName, Ethash& _sealEngine)
{
	//Test File
	js::mValue v;
	string s = contentsString(_testFileFullName);
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of 'difficulty.json' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);

	for (auto& i: v.get_obj())
	{
		js::mObject o = i.second.get_obj();
		cnote << "Difficulty test: " << i.first;
		BlockHeader parent;
		parent.setTimestamp(test::toInt(o["parentTimestamp"]));
		parent.setDifficulty(test::toInt(o["parentDifficulty"]));
		parent.setNumber(test::toInt(o["currentBlockNumber"]) - 1);

		BlockHeader current;
		current.setTimestamp(test::toInt(o["currentTimestamp"]));
		current.setNumber(test::toInt(o["currentBlockNumber"]));

		BOOST_CHECK_EQUAL(_sealEngine.calculateDifficulty(current, parent), test::toInt(o["currentDifficulty"]));
	}
}

BOOST_AUTO_TEST_SUITE(DifficultyTests)

BOOST_AUTO_TEST_CASE(difficultyTestsOlympic)
{
	test::TestOutputHelper::initTest();
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyOlimpic.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::Olympic)));

	if (dev::test::Options::get().fillTests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine);
}

BOOST_AUTO_TEST_CASE(difficultyTestsFrontier)
{
	test::TestOutputHelper::initTest();
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyFrontier.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::Frontier)));

	if (dev::test::Options::get().fillTests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine);
}

BOOST_AUTO_TEST_CASE(difficultyTestsMorden)
{
	test::TestOutputHelper::initTest();
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyMorden.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::Morden)));

	if (dev::test::Options::get().fillTests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine);
}

BOOST_AUTO_TEST_CASE(difficultyTestsHomestead)
{
	test::TestOutputHelper::initTest();
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyHomestead.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::HomesteadTest)));

	if (dev::test::Options::get().fillTests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine);
}

BOOST_AUTO_TEST_CASE(basicDifficultyTest)
{
	test::TestOutputHelper::initTest();
	string testPath = test::getTestPath();
	testPath += "/BasicTests/difficulty.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::Olympic)));

	testDifficulty(testPath, sealEngine);
}

BOOST_AUTO_TEST_SUITE_END()
