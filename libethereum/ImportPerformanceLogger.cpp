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
*  Class for logging of importing a Block into BlockChain.
*/

#include "ImportPerformanceLogger.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptors.hpp>

using namespace dev;
using namespace eth;

namespace
{

template <class ValueType>
static std::string pairToString(std::pair<std::string const, ValueType> const& _pair)
{
	return "\"" + _pair.first + "\": " + toString(_pair.second);
}

}

std::string ImportPerformanceLogger::constructReport(double _totalElapsed, std::unordered_map<std::string, std::string> const& _additionalValues)
{
	static std::string const Separator = ", ";

	std::string result;
	if (!_additionalValues.empty())
	{
		auto const keyValuesAdditional = _additionalValues | boost::adaptors::transformed(pairToString<std::string>);
		result += boost::algorithm::join(keyValuesAdditional, Separator);
		result += Separator;
	}

	m_stages.emplace("total", _totalElapsed);
	auto const keyValuesStages = m_stages | boost::adaptors::transformed(pairToString<double>);
	result += boost::algorithm::join(keyValuesStages, Separator);

	return result;
}


