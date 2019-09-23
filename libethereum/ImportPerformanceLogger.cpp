// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
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


