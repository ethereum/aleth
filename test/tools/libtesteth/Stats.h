// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <chrono>
#include <vector>

#include "TestHelper.h"

namespace dev
{
namespace test
{

class Stats: public Listener
{
public:
	using clock = std::chrono::high_resolution_clock;

	struct Item
	{
		clock::duration duration;
		int64_t gasUsed;
		std::string name;
	};

	static Stats& get();

	~Stats() final;

	void suiteStarted(std::string const& _name) override;
	void testStarted(std::string const& _name) override;
	void testFinished(int64_t _gasUsed) override;

private:
	clock::time_point m_tp;
	std::string m_currentSuite;
	std::string m_currentTest;
	std::vector<Item> m_stats;
};

}
}
