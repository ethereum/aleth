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
/** @file Miner.h
 * @author Gav Wood <i@gavwood.com>, Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <list>
#include <atomic>
#include <libethential/Common.h>
#include <libethcore/CommonEth.h>
#include <libethcore/Dagger.h>
#include <libethcore/OverlayDB.h>
#include "State.h"

namespace eth
{

class BlockChain;
class Client;

struct MineProgress
{
	double requirement;
	double best;
	double current;
	uint hashes;
	uint ms;
};

class Miner: public std::enable_shared_from_this<Miner>
{
public:
	/// Constructor.
	Miner(BlockChain const& _bc);
	
	/// Run miner with progress callback for completion status.
	void run(State _postMine, std::function<void(MineProgress _progress, bool _completed, State& _mined)> _progressCb);

	/// Stop mining.
	void stop();
	
	/// @returns if mining
	bool running();
	
	/// Restart mining.
	void restart(State _postMine);
	
	/// Check block validity prior to mining.
	bool paranoia() const { return m_paranoia; }
	/// Change whether we check block validity prior to mining.
	void setParanoia(bool _p) { m_paranoia = _p; }

	/// Check the progress of the mining.
	MineProgress miningProgress() const { return m_mineProgress; }
	/// Get and clear the mining history.
	std::list<MineInfo> miningHistory() { auto ret = m_mineHistory; m_mineHistory.clear(); return ret; }
	
private:
	/// Perform mining
	void mine();
	
	BlockChain const& m_bc;				///< Reference to BlockChain
	bool m_paranoia;					///< Toggle paranoia; if enabled, enacts and verifies state and block before mining
	
	State m_minerState;					///< State which is being mined

	MineInfo m_mineInfo;				///< Current Dagger mining information.
	MineProgress m_mineProgress;		///< Cumulative mining progress information.
	std::list<MineInfo> m_mineHistory;	///< History of Dagger mining information.
	
	std::unique_ptr<std::thread> m_run;	///< Thread which runs mine() and progress callback.
	std::recursive_mutex x_run;			///< Recursive mutex for startup, start/stup, and status of m_run thread.
	std::mutex m_restart;				///< Guards state of operational data members (primarily m_minerState).
	std::atomic<bool> m_stop;			///< Setting to false stops running thread in threadsafe manner.
};

}
