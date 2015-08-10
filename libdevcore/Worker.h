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
/** @file Worker.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "Guards.h"

namespace dev
{

enum class WorkerState
{
	Running,			///< thread started and running
	Stopped,			///< thread stopped, but still exists, ran be re-started
	Terminated			///< thread does not exist
};

class Worker: boost::noncopyable
{
protected:
	/// Creates a new worker initially in the "Killed" state.
	explicit Worker(std::string const& _name = "anon", unsigned _idleWaitMs = 30):
		m_name(_name),
		m_idleWaitMs(_idleWaitMs),
		m_state{ WorkerState::Terminated },
		m_requestedState{ WorkerState::Terminated }
	{}

	virtual ~Worker() { terminate(); }

	/// Starts worker thread; causes startedWorking() to be called.
	void startWorking();
	
	/// Stop worker thread; causes call to stopWorking().
	void stopWorking();

	/// Returns if worker thread is present and running.
	bool isWorking() const { return m_state == WorkerState::Running; }
	bool shouldStop() const { return m_requestedState != WorkerState::Running; }

	/// Called after thread is started from startWorking().
	virtual void startedWorking() {}
	
	/// Called continuously following sleep for m_idleWaitMs.
	virtual void doWork() {}

	/// Calls/should call doWork repeatedly until shouldStop is true.
	virtual void workLoop();
	
	/// Called when is to be stopped, just prior to thread being joined.
	virtual void doneWorking() {}

private:
	/// Stop and never start again.
	void terminate();

	std::string const m_name;

	unsigned const m_idleWaitMs = 0;
	
	mutable Mutex x_work;						///< Lock for the worker thread.
	std::unique_ptr<std::thread> m_work;		///< The worker thread.
	std::atomic<WorkerState> m_state;			///< The current state of the thread, controlled by the thread.
	std::atomic<WorkerState> m_requestedState;	///< The desired state of the thread, controlled from outside.
};

}
