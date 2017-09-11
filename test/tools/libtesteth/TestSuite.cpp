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

namespace dev
{
namespace test
{

void TestSute::runAllTestsInFolder(const std::string& _folder) const
{
	fs::path const fillersPath = fs::path(test::getTestPath()) / "src" / fs::path(folder() + "Filler") / fs::path(_folder);
	string const filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
	std::vector<boost::filesystem::path> const files = test::getJsonFiles(fillersPath.string(), filter);
	size_t fileCount = files.size();
	if (test::Options::get().filltests)
		fileCount *= 2; //tests are checked when filled and after they been filled

	fs::path const destTestFolder = fs::path(folder()) / fs::path(_folder);
	fs::path const srcTestFolder = fs::path(folder() + "Filler") / fs::path(_folder);

	auto suiteTestDo = [this](json_spirit::mValue const& _input, bool _fillin)
	{
		return doTests(_input, _fillin);
	};

	auto testOutput = dev::test::TestOutputHelper(fileCount);
	for (auto const& file: files)
	{
		test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
		executeTests(file.filename().string(), destTestFolder.string(), srcTestFolder.string(), suiteTestDo);
	}
}

}
}
