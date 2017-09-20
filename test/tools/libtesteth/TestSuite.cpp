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

}
}
