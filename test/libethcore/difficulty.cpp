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
#include <test/libtesteth/TestHelper.h>
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

void checkCalculatedDifficulty(BlockHeader const& _bi, BlockHeader const& _parent, Network _n, ChainOperationParams const& _p, string const& _testName = "")
{
	u256 difficulty = _bi.difficulty();
	u256 frontierDiff = _p.u256Param("homsteadForkBlock");

	//The ultimate formula (Homestead)
	if (_bi.number() > frontierDiff)
	{
		u256 minimumDifficulty = _p.u256Param("minimumDifficulty");
		bigint block_diff = _parent.difficulty();

		bigint a = (_parent.difficulty() / 2048);
		int b = 1 - int(_bi.timestamp() - _parent.timestamp()) / 10;
		bigint c = (_bi.number() / 100000) - 2;

		block_diff += a * max<int>(b, -99);
		block_diff += u256(1) << (unsigned)c;
		block_diff = max<bigint>(minimumDifficulty, block_diff);

		BOOST_CHECK_MESSAGE(difficulty == block_diff, "Homestead Check Calculated diff = " << difficulty << " expected diff = " << block_diff << _testName);
		return;
	}

	u256 durationLimit;
	u256 minimumDifficulty;
	u256 difficultyBoundDivisor;
	switch(_n)
	{
	case Network::MainNetwork:
	case Network::FrontierTest:
	case Network::HomesteadTest:
	case Network::Ropsten:
	case Network::TransitionnetTest:
		durationLimit = 13;
		minimumDifficulty = 131072;
		difficultyBoundDivisor = 2048;
	break;
	default:
		cerr << "testing undefined network difficulty";
		durationLimit = _p.u256Param("durationLimit");
		minimumDifficulty = _p.u256Param("minimumDifficulty");
		difficultyBoundDivisor = _p.u256Param("difficultyBoundDivisor");
		break;
	}

	//Frontier Era
	bigint block_diff = _parent.difficulty();

	bigint a = (_parent.difficulty() / difficultyBoundDivisor);
	bigint b = ((_bi.timestamp() - _parent.timestamp()) < durationLimit) ?  1 : -1;
	bigint c = (_bi.number() / 100000) - 2;

	block_diff += a * b;
	block_diff += u256(1) << (unsigned)c;
	block_diff = max<bigint>(minimumDifficulty, block_diff);

	BOOST_CHECK_MESSAGE(difficulty == block_diff, "Check Calculated diff = " << difficulty << " expected diff = " << block_diff << _testName);
	return;
}

void fillDifficulty(string const& _testFileFullName, Ethash& _sealEngine)
{
	int testN = 0;
	ostringstream finalTest;
	finalTest << "{" << std::endl;
	dev::test::TestOutputHelper::initTest(900);

	for (int stampDelta = 0; stampDelta < 45; stampDelta+=2)
	{
		for (u256 blockNumber = 1; blockNumber < 1500000; blockNumber += 25000)
		{
			testN++;
			json_spirit::mObject m;
			string testName = "DifficultyTest"+toString(testN);
			if (!dev::test::TestOutputHelper::passTest(m, testName))
				continue;

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
	string testFile = finalTest.str();
	testFile = testFile.replace(testFile.find_last_of(","), 1, "");
	writeFile(_testFileFullName, asBytes(testFile));
}

void testDifficulty(string const& _testFileFullName, Ethash& _sealEngine, Network _n)
{
	//Test File
	js::mValue v;
	string s = contentsString(_testFileFullName);
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of '" << _testFileFullName << "' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);
	dev::test::TestOutputHelper::initTest(v);

	for (auto& i: v.get_obj())
	{
		js::mObject o = i.second.get_obj();
		string testname = i.first;
		if (!dev::test::TestOutputHelper::passTest(o, testname))
			continue;

		BlockHeader parent;
		parent.setTimestamp(test::toInt(o["parentTimestamp"]));
		parent.setDifficulty(test::toInt(o["parentDifficulty"]));
		parent.setNumber(test::toInt(o["currentBlockNumber"]) - 1);

		BlockHeader current;
		current.setTimestamp(test::toInt(o["currentTimestamp"]));
		current.setNumber(test::toInt(o["currentBlockNumber"]));

		u256 difficulty = _sealEngine.calculateDifficulty(current, parent);
		current.setDifficulty(difficulty);
		BOOST_CHECK_EQUAL(difficulty, test::toInt(o["currentDifficulty"]));

		//Manual formula test
		checkCalculatedDifficulty(current, parent, _n, _sealEngine.chainParams(), "(" + i.first + ")");
	}
	dev::test::TestOutputHelper::finishTest();
}

BOOST_AUTO_TEST_SUITE(DifficultyTests)

BOOST_AUTO_TEST_CASE(difficultyTestsFrontier)
{
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyFrontier.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::FrontierTest)));

	if (dev::test::Options::get().filltests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine, Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(difficultyTestsRopsten)
{
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyRopsten.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::Ropsten)));

	if (dev::test::Options::get().filltests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine, Network::Ropsten);
}

BOOST_AUTO_TEST_CASE(difficultyTestsHomestead)
{
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyHomestead.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::HomesteadTest)));

	if (dev::test::Options::get().filltests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine, Network::HomesteadTest);
}

BOOST_AUTO_TEST_CASE(difficultyTestsMainNetwork)
{
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyMainNetwork.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::MainNetwork)));

	if (dev::test::Options::get().filltests)
		fillDifficulty(testFileFullName, sealEngine);

	testDifficulty(testFileFullName, sealEngine, Network::MainNetwork);
}

BOOST_AUTO_TEST_CASE(difficultyTestsCustomMainNetwork)
{
	string testFileFullName = test::getTestPath();
	testFileFullName += "/BasicTests/difficultyCustomMainNetwork.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::MainNetwork)));

	if (dev::test::Options::get().filltests)
	{
		u256 homsteadBlockNumber = 1000000;
		std::vector<u256> blockNumberVector = {homsteadBlockNumber - 100000, homsteadBlockNumber, homsteadBlockNumber + 100000};
		std::vector<u256> parentDifficultyVector = {1000, 2048, 4000, 1000000};
		std::vector<int> timestampDeltaVector = {0, 1, 8, 10, 13, 20, 100, 800, 1000, 1500};

		int testN = 0;
		ostringstream finalTest;
		finalTest << "{" << std::endl;

		for (size_t bN = 0; bN < blockNumberVector.size(); bN++)
			for (size_t pdN = 0; pdN < parentDifficultyVector.size(); pdN++)
				for (size_t tsN = 0; tsN < timestampDeltaVector.size(); tsN++)
				{
					testN++;
					int stampDelta = timestampDeltaVector.at(tsN);
					u256 blockNumber = blockNumberVector.at(bN);
					u256 pDiff = parentDifficultyVector.at(pdN);

					u256 pStamp = dev::test::RandomCode::randomUniInt();
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
					replaceMap["[CDIFF]"] = toCompactHex(sealEngine.calculateDifficulty(current, parent), HexPrefix::Add);

					dev::test::RandomCode::parseTestWithTypes(tmptest, replaceMap);
					finalTest << tmptest;
				}

		finalTest << std::endl << "}";
		string testFile = finalTest.str();
		testFile = testFile.replace(testFile.find_last_of(","), 1, "");
		writeFile(testFileFullName, asBytes(testFile));
	}

	testDifficulty(testFileFullName, sealEngine, Network::MainNetwork);
}

BOOST_AUTO_TEST_CASE(basicDifficultyTest)
{
	string testPath = test::getTestPath();
	testPath += "/BasicTests/difficulty.json";

	Ethash sealEngine;
	sealEngine.setChainParams(ChainParams(genesisInfo(Network::MainNetwork)));

	testDifficulty(testPath, sealEngine, Network::MainNetwork);
}

BOOST_AUTO_TEST_SUITE_END()
