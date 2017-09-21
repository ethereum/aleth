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
 * A base class for test suites
 */

#pragma once
#include <boost/filesystem/path.hpp>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>

namespace dev
{
namespace test
{

class TestSuite
{
protected:
	// A folder of the test suite. like "VMTests". should be implemented for each test suite.
	virtual std::string suiteFolder() const = 0;

	// A folder of the test suite in src folder. like "VMTestsFiller". should be implemented for each test suite.
	virtual std::string suiteFillerFolder() const = 0;

public:
	virtual ~TestSuite() {}

	// Main test executive function. should be declared for each test suite. it fills and runs the test .json file
	virtual json_spirit::mValue doTests(json_spirit::mValue const&, bool) const = 0;

	// Execute all tests from _folder
	void runAllTestsInFolder(std::string const& _testFolder) const;

	// Copy .json tests from the src folder to the dest folder because such test is crafted manually and could not be filled.
	// Used in bcForgedTest, ttWrongRLP tests and such.
	void copyAllTestsFromFolder(std::string const& _testFolder) const;

	// Return full path to folder for tests from _testFolder
	boost::filesystem::path getFullPathFiller(std::string const& _testFolder) const;

	// Structure  <suiteFolder>/<testFolder>/<test>.json
	boost::filesystem::path getFullPath(std::string const& _testFolder) const;

	void executeTests(const std::string& _name, boost::filesystem::path const& _testPathAppendix, boost::filesystem::path const& _fillerPathAppendix, std::function<json_spirit::mValue(json_spirit::mValue const&, bool)> doTests) const;
};

}
}
