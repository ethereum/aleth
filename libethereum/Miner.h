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

/**
 * @brief Implements Miner.
 * The miner will start a thread when there is work. When a block is found the
 * _progressCb is called and is expected to restart the miner with new work when
 * appropriate. _progressCb is also called every ~100ms with progress updates.
 */
class Miner: public std::enable_shared_from_this<Miner>
{
public:
	/// Constructor. Progress callback is expected to call completeMine() and restart().
	Miner(BlockChain& _bc, std::function<void(MineProgress _progress, bool _completed)> _progressCb);

	/// Stop mining.
	void stop();
	
	/// @returns if mining
	bool running();
	
	/// (Re)start mining.
	void restart(State _postMine);
	
	/// Check block validity prior to mining.
	bool paranoia() const { return m_paranoia; }
	/// Change whether we check block validity prior to mining.
	void setParanoia(bool _p) { m_paranoia = _p; }

	/// Check the progress of the mining.
	MineProgress miningProgress() const { return m_mineProgress; }
	/// Get and clear the mining history.
	std::list<MineInfo> miningHistory() { auto ret = m_mineHistory; m_mineHistory.clear(); return ret; }

	/// Imports completed block; called within _progressCb after mining is complete.
	h256s completeMine(OverlayDB& _stateDB);
	
private:
	/// Called by restart; performs mining within thread and starts new thread when necessary
	void mine();
	
	BlockChain &m_bc;						///< BlockChain required by state when committing to mine block.
	std::function<void(MineProgress _progress, bool _completed)> m_progressCb;	///< Called periodically and when mining is complete.

	bool m_paranoia;						///< Toggle paranoia; if enabled, enacts and verifies state and block before mining.

	// Shared pointers are used for state, as State does not copy block bytes which are
	// needed for subsequent call to completeMine() and they're thread-safe.
	std::shared_ptr<State> m_minerState;	///< State which is being mined.
	std::shared_ptr<State> m_minedState;	///< State after mined block reaches required difficulty.

	MineInfo m_mineInfo;				///< Current Dagger mining information.
	MineProgress m_mineProgress;		///< Cumulative mining progress information.
	std::list<MineInfo> m_mineHistory;	///< History of Dagger mining information.
	
	std::mutex x_run;					///< Guards m_run thread
	std::unique_ptr<std::thread> m_run;	///< Thread which runs mine() and progress callback.
	std::atomic<bool> m_stop;			///< Signals m_run thread to exit
};

}
