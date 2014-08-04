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

void Miner::run(State _s, std::function<void(MineProgress _progress, bool _completed, State& _mined)> _progress_cb)
{
	restart(_s);
	
	const char* c_threadName = "mine";
	if (!m_run.get())
		m_run.reset(new thread([&, c_threadName, _progress_cb]()
		{
		   m_stop = false;
		   setThreadName(c_threadName);
		   while(!m_stop)
		   {
			   lock_guard<std::mutex> ml(x_restart);
			   mine();

			   _progress_cb(m_mineProgress, m_mineInfo.completed, std::ref(m_minerState));

			   if (m_mineInfo.completed)
				   m_stop = true;
		   }
		}));
}

bool Miner::running()
{
	// cleanup thread
	if (m_stop && m_run.get())
		stop();
	
	return m_stop ? false : !!m_run.get();
}

void Miner::restart(State _s)
{
	lock_guard<std::mutex> l(x_restart);
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
			m_stop = true;
		}
	}
	else
		m_minerState.commitToMine(m_bc);
}

void Miner::stop()
{
	m_stop = true;
	if (m_run){ m_run->join(); m_run = nullptr; }
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
