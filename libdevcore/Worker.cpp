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
/** @file Worker.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Worker.h"

#include <chrono>
#include <thread>
#include "Log.h"
using namespace std;
using namespace dev;

void Worker::startWorking()
{
//	cnote << "startWorking for thread" << m_name;
	std::unique_lock<std::mutex> l(x_work);
	assert (m_state != WorkerState::Constructor); // Vptr table should be ready here! Have you called allowVprAccess();?
	if (m_work)
	{
		WorkerState ex = WorkerState::Stopped;
		m_state.compare_exchange_strong(ex, WorkerState::Starting);
		m_state_notifier.notify_all();
	}
	else
	{
		m_work.reset(new thread([&]()
		{
			setThreadName(m_name.c_str());
//			cnote << "Thread begins";
			{
				std::unique_lock<std::mutex> l(x_work);
				while (m_state == WorkerState::Constructor) // The constructor is still running, so cannot access vptrs.
					m_state_notifier.wait(l);
			}
			while (m_state != WorkerState::Killing)
			{
				WorkerState ex = WorkerState::Starting;
				{
					// the condition variable-related lock
					unique_lock<mutex> l(x_work);
					m_state = WorkerState::Started;
				}
//				cnote << "Trying to set Started: Thread was" << (unsigned)ex << "; " << ok;
				m_state_notifier.notify_all();

				try
				{
					startedWorking();
					workLoop();
					doneWorking();
				}
				catch (std::exception const& _e)
				{
					clog(WarnChannel) << "Exception thrown in Worker thread: " << _e.what();
				}

//				ex = WorkerState::Stopping;
//				m_state.compare_exchange_strong(ex, WorkerState::Stopped);

				{
					// the condition variable-related lock
					unique_lock<mutex> l(x_work);
					ex = m_state.exchange(WorkerState::Stopped);
//					cnote << "State: Stopped: Thread was" << (unsigned)ex;
					if (ex == WorkerState::Killing || ex == WorkerState::Starting)
						m_state.exchange(ex);
				}
				m_state_notifier.notify_all();
//				cnote << "Waiting until not Stopped...";

				{
					unique_lock<mutex> l(x_work);
					DEV_TIMED_ABOVE("Worker stopping", 100)
						while (m_state == WorkerState::Stopped)
							m_state_notifier.wait(l);
				}
			}
		}));
//		cnote << "Spawning" << m_name;
	}
	DEV_TIMED_ABOVE("Start worker", 100)
		while (m_state == WorkerState::Starting)
			m_state_notifier.wait(l); // waiting
}

void Worker::stopWorking()
{
	std::unique_lock<Mutex> l(x_work);
	if (m_work)
	{
		WorkerState ex = WorkerState::Started;
		m_state.compare_exchange_strong(ex, WorkerState::Stopping);
		m_state_notifier.notify_all();

		DEV_TIMED_ABOVE("Stop worker", 100)
			while (m_state != WorkerState::Stopped)
				m_state_notifier.wait(l); // but yes who can wake this up, when the mutex is taken.
	}
}

void Worker::terminate()
{
//	cnote << "stopWorking for thread" << m_name;
	std::unique_lock<Mutex> l(x_work);
	if (m_work)
	{
		m_state.exchange(WorkerState::Killing);
		l.unlock();
		m_state_notifier.notify_all();
		DEV_TIMED_ABOVE("Terminate worker", 100)
			m_work->join();

		l.lock();
		m_work.reset();
	}
}

void Worker::allowVptrAccess()
{
	Guard l(x_work);
	m_state = WorkerState::Starting;
	m_state_notifier.notify_all();
}

void Worker::workLoop()
{
	while (m_state == WorkerState::Started)
	{
		if (m_idleWaitMs)
			this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
		doWork();
	}
}
