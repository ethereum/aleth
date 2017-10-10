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
 * Base functions for all test suites
 */

#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestSuite.h>
#include <test/tools/libtesteth/Stats.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>
#include <string>
using namespace std;
using namespace dev;
namespace fs = boost::filesystem;

//Helper functions for test proccessing
namespace {

void removeComments(json_spirit::mValue& _obj)
{
	if (_obj.type() == json_spirit::obj_type)
	{
		list<string> removeList;
		for (auto& i: _obj.get_obj())
		{
			if (i.first.substr(0, 2) == "//")
			{
				removeList.push_back(i.first);
				continue;
			}

			removeComments(i.second);
		}
		for (auto& i: removeList)
			_obj.get_obj().erase(_obj.get_obj().find(i));
	}
	else if (_obj.type() == json_spirit::array_type)
	{
		for (auto& i: _obj.get_array())
			removeComments(i);
	}
}

void addClientInfo(json_spirit::mValue& _testDestination, json_spirit::mValue const& _testSource, fs::path const& _testSourcePath, h256 const& _testSourceHash)
{
	for (auto& i: _testDestination.get_obj())
	{
		json_spirit::mObject& o = i.second.get_obj();
		json_spirit::mObject clientinfo;

		string comment;
		// copy comment from test source if there is one
		for (auto& j: _testSource.get_obj())
		{
			json_spirit::mObject const& o2 = j.second.get_obj();
			if (o2.count("_info") && j.first == i.first)
			{
				json_spirit::mObject const& existingInfo = o2.at("_info").get_obj();
				if (existingInfo.count("comment"))
					comment = existingInfo.at("comment").get_str();
				break;
			}
		}

		clientinfo["filledwith"] = test::prepareVersionString();
		clientinfo["lllcversion"] = test::prepareLLLCVersionString();
		clientinfo["source"] = _testSourcePath.string();
		clientinfo["sourceHash"] = toString(_testSourceHash);
		clientinfo["comment"] = comment;
		o["_info"] = clientinfo;
	}
}

void checkFillerHash(fs::path const& _compiledTest, fs::path const& _sourceTest)
{
	json_spirit::mValue v;
	string const s = asString(dev::contents(_compiledTest));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + _compiledTest.string() + " is empty.");
	json_spirit::read_string(s, v);
	h256 const fillerHash = sha3(dev::contents(_sourceTest));

	for (auto& i: v.get_obj())
	{
		BOOST_REQUIRE_MESSAGE(i.second.type() == json_spirit::obj_type, i.first + " should contain an object under a test name.");
		json_spirit::mObject const& obj = i.second.get_obj();
		BOOST_REQUIRE_MESSAGE(obj.count("_info") > 0, "_info section not set! " + _compiledTest.string());
		json_spirit::mObject const& info = obj.at("_info").get_obj();
		BOOST_REQUIRE_MESSAGE(info.count("sourceHash") > 0, "sourceHash not found in " + _compiledTest.string() + " in " + i.first);
		h256 const sourceHash = h256(info.at("sourceHash").get_str());
		BOOST_CHECK_MESSAGE(sourceHash == fillerHash, "Test " + _compiledTest.string() + " in " + i.first + " is outdated. Filler hash is different!");
	}
}

}

namespace dev
{
namespace test
{

string const c_fillerPostf = "Filler";
string const c_copierPostf = "Copier";

void TestSuite::runAllTestsInFolder(string const& _testFolder) const
{
	// check that destination folder test files has according Filler file in src folder
	vector<fs::path> const compiledFiles = test::getJsonFiles(getFullPath(_testFolder));
	for (auto const& file: compiledFiles)
	{
		fs::path const expectedFillerName = getFullPathFiller(_testFolder) / fs::path(file.stem().string() + c_fillerPostf + ".json");
		fs::path const expectedCopierName = getFullPathFiller(_testFolder) / fs::path(file.stem().string() + c_copierPostf + ".json");
		BOOST_REQUIRE_MESSAGE(fs::exists(expectedFillerName) || fs::exists(expectedCopierName), "Compiled test folder contains test without Filler: " + file.filename().string());
		BOOST_REQUIRE_MESSAGE(!(fs::exists(expectedFillerName) && fs::exists(expectedCopierName)), "Src test could either be Filler.json or Copier.json: " + file.filename().string());

		// Check that filled tests created from actual fillers
		if (Options::get().filltests == false)
		{
			if (fs::exists(expectedFillerName))
				checkFillerHash(file, expectedFillerName);
			if (fs::exists(expectedCopierName))
				checkFillerHash(file, expectedCopierName);
		}
	}

	// run all tests
	string const filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
	vector<fs::path> const files = test::getJsonFiles(getFullPathFiller(_testFolder).string(), filter);

	auto testOutput = dev::test::TestOutputHelper(files.size());
	for (auto const& file: files)
	{
		testOutput.showProgress();
		test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
		executeTest(_testFolder, file);
	}
}

fs::path TestSuite::getFullPathFiller(string const& _testFolder) const
{
	return test::getTestPath() / "src" / suiteFillerFolder() / _testFolder;
}

fs::path TestSuite::getFullPath(string const& _testFolder) const
{
	return test::getTestPath() / suiteFolder() / _testFolder;
}

void TestSuite::executeTest(string const& _testFolder, fs::path const& _jsonFileName) const
{
	fs::path const boostRelativeTestPath = fs::relative(_jsonFileName, getTestPath());
	string testname = _jsonFileName.stem().string();
	bool isCopySource = false;
	if (testname.rfind(c_fillerPostf) != string::npos)
		testname = testname.substr(0, testname.rfind("Filler"));
	else if (testname.rfind(c_copierPostf) != string::npos)
	{
		testname = testname.substr(0, testname.rfind(c_copierPostf));
		isCopySource = true;
	}
	else
		BOOST_REQUIRE_MESSAGE(false, "Incorrect file suffix in the filler folder! " + _jsonFileName.string());

	// Filename of the test that would be generated
	fs::path const boostTestPath = getFullPath(_testFolder) / fs::path(testname + ".json");

	// TODO: An old unmaintained way to gather execution stats needs review.
	if (Options::get().stats)
		Listener::registerListener(Stats::get());

	if (Options::get().filltests)
	{
		if (isCopySource)
		{
			clog << "Copying " << _jsonFileName.string() << "\n";
			clog << " TO " << boostTestPath.string() << "\n";
			assert(_jsonFileName.string() != boostTestPath.string());
			TestOutputHelper::showProgress();
			dev::test::copyFile(_jsonFileName, boostTestPath);
			BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(boostTestPath.string()), "Error when copying the test file!");

			// Update _info and build information of the copied test
			json_spirit::mValue v;
			string const s = asString(dev::contents(boostTestPath));
			json_spirit::read_string(s, v);
			addClientInfo(v, v, boostRelativeTestPath, sha3(dev::contents(_jsonFileName)));
			writeFile(boostTestPath, asBytes(json_spirit::write_string(v, true)));
		}
		else
		{
			if (!Options::get().singleTest)
				cnote << "Populating tests...";

			json_spirit::mValue v;
			bytes const byteContents = dev::contents(_jsonFileName);
			string const s = asString(byteContents);
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + _jsonFileName.string() + " is empty.");

			json_spirit::read_string(s, v);
			removeComments(v);
			json_spirit::mValue output = doTests(v, true);
			addClientInfo(output, v, boostRelativeTestPath, sha3(byteContents));
			writeFile(boostTestPath, asBytes(json_spirit::write_string(output, true)));
		}
	}

	// Test is generated. Now run it and check that there should be no errors
	if ((Options::get().singleTest && Options::get().singleTestName == testname) || !Options::get().singleTest)
		cnote << "TEST " << testname << ":";

	json_spirit::mValue v;
	string const s = asString(dev::contents(boostTestPath.string()));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " << boostTestPath.string() << " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
	json_spirit::read_string(s, v);
	Listener::notifySuiteStarted(testname);
	doTests(v, false);
}

}
}
