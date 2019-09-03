/*
This file is part of aleth.

aleth is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

aleth is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file StateTests.h
 * StateTests functions.
 */

#pragma once
#include <test/tools/libtesteth/TestSuite.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace test
{

class StateTestSuite: public TestSuite
{
public:
	json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
	boost::filesystem::path suiteFolder() const override;
	boost::filesystem::path suiteFillerFolder() const override;
};

}
}
