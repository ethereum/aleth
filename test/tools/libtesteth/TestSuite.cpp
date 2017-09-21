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

namespace dev
{
namespace test
{

void removeComments(json_spirit::mValue& _obj)
{
	if (_obj.type() == json_spirit::obj_type)
	{
		std::list<string> removeList;
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

void addClientInfo(json_spirit::mValue& _v, fs::path const& _testSource)
{
	for (auto& i: _v.get_obj())
	{
		json_spirit::mObject& o = i.second.get_obj();
		json_spirit::mObject clientinfo;

		string comment;
		if (o.count("_info"))
		{
			json_spirit::mObject& existingInfo = o["_info"].get_obj();
			if (existingInfo.count("comment"))
				comment = existingInfo["comment"].get_str();
		}

		clientinfo["filledwith"] = prepareVersionString();
		clientinfo["lllcversion"] = prepareLLLCVersionString();
		clientinfo["source"] = _testSource.string();
		clientinfo["comment"] = comment;
		o["_info"] = clientinfo;
	}
}

void TestSuite::runAllTestsInFolder(string const& _testFolder) const
{
	string const filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
	std::vector<boost::filesystem::path> const files = test::getJsonFiles(getFullPathFiller(_testFolder).string(), filter);
	size_t fileCount = files.size();
	if (test::Options::get().filltests)
		fileCount *= 2; //tests are checked when filled and after they been filled

	fs::path const destTestFolder = fs::path(suiteFolder()) / _testFolder;
	fs::path const srcTestFolder = fs::path(suiteFillerFolder()) / _testFolder;

	auto suiteTestDo = [this](json_spirit::mValue const& _input, bool _fillin)
	{
		return doTests(_input, _fillin);
	};

	auto testOutput = dev::test::TestOutputHelper(fileCount);
	for (auto const& file: files)
	{
		testOutput.showProgress();
		test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
		executeTests(file.filename().string(), destTestFolder.string(), srcTestFolder.string(), suiteTestDo);
	}
}

void TestSuite::copyAllTestsFromFolder(string const& _testFolder) const
{
	std::vector<fs::path> const files = test::getJsonFiles(getFullPathFiller(_testFolder).string());
	for (auto const& file: files)
	{
		fs::path const destFile = getFullPath(_testFolder) / file.filename().string();
		fs::path const srcFile = getFullPathFiller(_testFolder) / file.filename().string();
		clog << "Copying " << srcFile.string() << "\n";
		clog << " TO " << destFile.string() << "\n";
		assert(srcFile.string() != destFile.string());
		auto testOutput = dev::test::TestOutputHelper();
		testOutput.showProgress();
		dev::test::copyFile(srcFile.string(), destFile.string());
		BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(destFile.string()), "Error when copying the test file!");
	}
	runAllTestsInFolder(_testFolder); //check that copied tests are valid
}

fs::path TestSuite::getFullPathFiller(string const& _testFolder) const
{
	return fs::path(test::getTestPath()) / "src" / suiteFillerFolder() / _testFolder;
}

fs::path TestSuite::getFullPath(string const& _testFolder) const
{
	return fs::path(test::getTestPath()) / suiteFolder() / _testFolder;
}

void TestSuite::executeTests(const string& _name, fs::path const& _testPathAppendix, fs::path const& _fillerPathAppendix, std::function<json_spirit::mValue(json_spirit::mValue const&, bool)> doTests) const
{
	fs::path const testPath = getTestPath() / _testPathAppendix;

	if (Options::get().stats)
		Listener::registerListener(Stats::get());

	//Get the test name
	string name = _name;
	if (_name.rfind("Filler.json") != std::string::npos)
		name = _name.substr(0, _name.rfind("Filler.json"));
	else if (_name.rfind(".json") != std::string::npos)
		name = _name.substr(0, _name.rfind(".json"));

	if (Options::get().filltests)
	{
		if (!Options::get().singleTest)
			cnote << "Populating tests...";
		json_spirit::mValue v;
		boost::filesystem::path p(__FILE__);

		string const nameEnding = "Filler.json";
		fs::path const testfileUnderTestPath = fs::path ("src") / _fillerPathAppendix / fs::path(name + nameEnding);
		fs::path const testfilename = getTestPath() / testfileUnderTestPath;
		string s = asString(dev::contents(testfilename));
		BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + testfilename.string() + " is empty.");

		json_spirit::read_string(s, v);
		removeComments(v);
		json_spirit::mValue output = doTests(v, true);
		addClientInfo(output, testfileUnderTestPath);
		writeFile(testPath / fs::path(name + ".json"), asBytes(json_spirit::write_string(output, true)));
	}

	if ((Options::get().singleTest && Options::get().singleTestName == name) || !Options::get().singleTest)
		cnote << "TEST " << name << ":";

	json_spirit::mValue v;
	string s = asString(dev::contents(testPath / fs::path(name + ".json")));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " << (testPath / fs::path(name + ".json")).string() << " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
	json_spirit::read_string(s, v);
	Listener::notifySuiteStarted(name);
	doTests(v, false);
}
}
}
