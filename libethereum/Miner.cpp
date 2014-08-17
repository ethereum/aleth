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
/** @file Miner.cpp
 * @author Gav Wood <i@gavwood.com>, Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include "Miner.h"
#include "Client.h"
#include "State.h"

using namespace std;
using namespace eth;

Miner::Miner(BlockChain const& _bc):
	m_bc(_bc),
	m_paranoia(false),
	m_stop(true)
{}

void Miner::run(State _s, std::function<void(MineProgress _progress, bool _completed, State& _mined)> _progressCb)
{
	restart(_s);
	
	const char* c_threadName = "mine";
	std::lock_guard<std::recursive_mutex> l(x_run);
	if (!m_run.get())
		m_run.reset(new thread([&, c_threadName, _progressCb]()
		{
			setThreadName(c_threadName);
			m_stop.store(false, std::memory_order_release);
			while (!m_stop.load(std::memory_order_acquire))
			{
				lock_guard<std::mutex> ml(m_restart);
				mine();

				_progressCb(m_mineProgress, m_mineInfo.completed, std::ref(m_minerState));

				if (m_mineInfo.completed)
				   m_stop.store(true, std::memory_order_release);
			}
		}));
}

bool Miner::running()
{
	std::lock_guard<std::recursive_mutex> l(x_run);
	
	// Cleanup thread if bad-block caused exit.
	if (m_stop && m_run.get())
		stop();
	
	return m_stop ? false : !!m_run.get();
}

void Miner::restart(State _s)
{
	lock_guard<std::mutex> l(m_restart);
	m_minerState = _s;
	
	m_mineProgress.best = (double)-1;
	m_mineProgress.hashes = 0;
	m_mineProgress.ms = 0;
	
	if (m_paranoia)
	{
		if (m_minerState.amIJustParanoid(m_bc))
		{
			cnote << "I'm just paranoid. Block is fine.";
			m_minerState.commitToMine(m_bc);
		}
		else
		{
			cwarn << "I'm not just paranoid. Cannot mine. Please file a bug report.";
			m_stop.store(true, std::memory_order_release);
		}
	}
	else
		m_minerState.commitToMine(m_bc);
}

void Miner::stop()
{
	std::lock_guard<std::recursive_mutex> l(x_run);
	m_stop.store(true, std::memory_order_release);
	if (m_run.get())
		m_run->join();
	m_run = nullptr;
};

void Miner::mine()
{
	cwork << "MINE";

	// Mine for a while.
	m_mineInfo = m_minerState.mine(100);
	m_mineProgress.best = min(m_mineProgress.best, m_mineInfo.best);
	m_mineProgress.current = m_mineInfo.best;
	m_mineProgress.requirement = m_mineInfo.requirement;
	m_mineProgress.ms += 100;
	m_mineProgress.hashes += m_mineInfo.hashes;
	m_mineHistory.push_back(MineInfo(m_mineInfo));
}
