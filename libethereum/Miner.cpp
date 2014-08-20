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

Miner::Miner(BlockChain& _bc, std::function<void(MineProgress _progress, bool _completed)> _progressCb):
	m_bc(_bc),
	m_progressCb(_progressCb),
	m_paranoia(false),
	m_mineProgress{(double)-1,0,0,0,0},
	m_stop(true)
{}

void Miner::restart(State _s)
{
	State *s = new State(_s);
	if (m_paranoia)
	{
		if (s->amIJustParanoid(m_bc))
			cnote << "I'm just paranoid. Block is fine.";
		else
		{
			cwarn << "I'm not just paranoid. Cannot mine. Please file a bug report.";
			stop();
			return;
		}
	}
	s->commitToMine(m_bc);
	m_minerState = std::shared_ptr<State>(s);
	m_minedState.reset();
	
	m_mineProgress.best = (double)-1;
	m_mineProgress.hashes = 0;
	m_mineProgress.ms = 0;
	
	mine();
}

bool Miner::running()
{
	return !m_stop;
}

void Miner::stop()
{
	std::lock_guard<std::mutex> l(x_run);
	m_stop.store(true, std::memory_order_release);
	m_run = nullptr;
}

void Miner::mine()
{
	const char* c_threadName = "mine";
	std::lock_guard<std::mutex> l(x_run);
	m_stop.store(false, std::memory_order_release);
	if (!m_run.get())
	{
		m_run.reset(new thread([&, c_threadName]()
			{
				setThreadName(c_threadName);
				while (!m_stop.load(std::memory_order_acquire))
				{
					std::shared_ptr<State> s = m_minerState;
					if (s.get())
					{
						cwork << "MINE";
						m_mineInfo = s->mine(100);
						m_mineProgress.best = min(m_mineProgress.best, m_mineInfo.best);
						m_mineProgress.current = m_mineInfo.best;
						m_mineProgress.requirement = m_mineInfo.requirement;
						m_mineProgress.ms += 100;
						m_mineProgress.hashes += m_mineInfo.hashes;
						m_mineHistory.push_back(MineInfo(m_mineInfo));
						
						if (m_mineInfo.completed)
						{
							cwork << "COMPLETE MINE";
							m_minedState = m_minerState;
							State *prev = m_minerState.get();
							m_progressCb(m_mineProgress, m_mineInfo.completed);
							
							// Stop if miner state wasn't changed by restart()
							if (prev == m_minerState.get())
								stop();
						}
						else
							m_progressCb(m_mineProgress, m_mineInfo.completed);
					}
				}
			}));
		m_run->detach();
	}
}

h256s Miner::completeMine(OverlayDB& _stateDB)
{
	h256s ret = h256s();
	std::shared_ptr<State> s = m_minedState;
	if (m_minedState.get() != nullptr)
	{
		m_minedState->completeMine();
		ret = m_bc.attemptImport(m_minedState->blockData(), _stateDB);
	}
	
	return ret;
}

