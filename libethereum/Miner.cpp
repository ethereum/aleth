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

Miner::Miner():
	m_stop(true),
	m_restart(false)
{
}

void Miner::run(BlockChain const& _bc, State &_postMine, std::function<void(MineProgress _progress, bool _completed, State &_postMined)> _progress_cb)
{
	assert(_progress_cb);
//	std::function<void(MineProgress _progress, bool _completed, State &_postMined)> cb = _progress_cb;
	
	m_currentMine = _postMine;
	const char* c_threadName = "mine";
	if (!m_run)
		m_run.reset(new thread([&, _progress_cb]()
		{
		   m_stop = false;
		   setThreadName(c_threadName);
		   while(!m_stop)
		   {
			   // todo: replace m_restart w/conditional or future
			   if (m_restart)
			   {
				   m_restart = false;
				   m_currentMine = m_restartMine;
			   }
			   
			   if (mine(_bc, m_currentMine))
				   _progress_cb(m_mineProgress, true, m_currentMine);
			   else
				   _progress_cb(m_mineProgress, false, m_currentMine);
		   }
		}));
}

void Miner::restart(State _postMine)
{
	m_restartMine = _postMine;
	m_restart = true;
}

bool Miner::mine(BlockChain const& _bc, State &_postMine, bool _restart)
{
	if(_restart)
	{
		m_mineProgress.best = (double)-1;
		m_mineProgress.hashes = 0;
		m_mineProgress.ms = 0;

		// todo: fixme, WriteGuard l(x_stateDB)
		
		if (m_paranoia)
		{
			if (_postMine.amIJustParanoid(_bc))
			{
				cnote << "I'm just paranoid. Block is fine.";
				_postMine.commitToMine(_bc);
			}
			else
			{
				cwarn << "I'm not just paranoid. Cannot mine. Please file a bug report.";
				// todo: disable mining
	//			m_doMine = false;
			}
		}
		else
			_postMine.commitToMine(_bc);
	}
	
	cwork << "MINE";

	// Mine for a while.
	MineInfo mineInfo = _postMine.mine(100);
	m_mineProgress.best = min(m_mineProgress.best, mineInfo.best);
	m_mineProgress.current = mineInfo.best;
	m_mineProgress.requirement = mineInfo.requirement;
	m_mineProgress.ms += 100;
	m_mineProgress.hashes += mineInfo.hashes;
	m_mineHistory.push_back(mineInfo);
	return mineInfo.completed;
}
