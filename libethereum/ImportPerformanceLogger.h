// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Class for logging of importing a Block into BlockChain.
#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Log.h>

#include <unordered_map>
#include <string>

namespace dev
{

namespace eth
{

class ImportPerformanceLogger
{
public:
	void onStageFinished(std::string const& _name)
	{
		m_stages[_name] = m_stageTimer.elapsed();
		m_stageTimer.restart();
	}

	double stageDuration(std::string const& _name) const
	{
		auto const it = m_stages.find(_name);
		return it != m_stages.end() ? it->second : 0;
	}

	void onFinished(std::unordered_map<std::string, std::string> const& _additionalValues)
	{
		double const totalElapsed = m_totalTimer.elapsed();
		if (totalElapsed > 0.5)
		{
            cdebug << "SLOW IMPORT: { " << constructReport(totalElapsed, _additionalValues) << " }";
        }
	}

private:
	std::string constructReport(double _totalElapsed, std::unordered_map<std::string, std::string> const& _additionalValues);

	Timer m_totalTimer;
	Timer m_stageTimer;
	std::unordered_map<std::string, double> m_stages;
};

}
}
