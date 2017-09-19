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
#include <boost/filesystem/path.hpp>
#include <string>
namespace fs = boost::filesystem;
using namespace std;
using namespace dev;

namespace
{
	// Structure  <suiteFolder>/<testFolder>/<test>.json
	// Return full path to folder for tests from _testFolder
	fs::path getFullPathFiller(string const& _suiteFolder, string const& _testFolder)
	{
		return fs::path(test::getTestPath()) / "src" / _suiteFolder / _testFolder;
	}

	fs::path getFullPathTest(string const& _suiteFolder, string const& _testFolder)
	{
		return fs::path(test::getTestPath()) / _suiteFolder / _testFolder;
	}
}

namespace dev
{
namespace test
{

void TestSuite::runAllTestsInFolder(string const& _testFolder) const
{
	string const filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
	std::vector<boost::filesystem::path> const files = test::getJsonFiles(getFullPathFiller(suiteFillerFolder(), _testFolder).string(), filter);
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
	std::vector<fs::path> const files = test::getJsonFiles(getFullPathFiller(suiteFillerFolder(), _testFolder).string());
	for (auto const& file: files)
	{
		fs::path const destFile = getFullPathTest(suiteFolder(), _testFolder) / file.filename().string();
		fs::path const srcFile = getFullPathFiller(suiteFillerFolder(), _testFolder) / file.filename().string();
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

}
}
