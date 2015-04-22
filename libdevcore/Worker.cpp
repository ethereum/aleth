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
	Guard l(x_work);
	m_state = WorkerState::Starting;
	if (!m_work)
	{
		m_work.reset(new thread([&]()
		{
			setThreadName(m_name.c_str());
			while (m_state != WorkerState::Killing)
			{
				WorkerState ex = WorkerState::Starting;
				m_state.compare_exchange_strong(ex, WorkerState::Started);

				startedWorking();
				cnote << "Entering work loop...";
				workLoop();
				cnote << "Finishing up worker thread...";
				doneWorking();

//				ex = WorkerState::Stopping;
//				m_state.compare_exchange_strong(ex, WorkerState::Stopped);

				ex = m_state.exchange(WorkerState::Stopped);
				if (ex == WorkerState::Killing)
					m_state.exchange(ex);

				while (m_state == WorkerState::Stopped)
					this_thread::sleep_for(chrono::milliseconds(20));
			}
		}));
		cnote << "Spawning" << m_name;
	}
	while (m_state != WorkerState::Started)
		this_thread::sleep_for(chrono::microseconds(20));
}

void Worker::stopWorking()
{
//	cnote << "stopWorking for thread" << m_name;
	ETH_GUARDED(x_work)
		if (m_work)
		{
			cnote << "Stopping" << m_name;
			WorkerState ex = WorkerState::Started;
			m_state.compare_exchange_strong(ex, WorkerState::Stopping);

			while (m_state != WorkerState::Stopped)
				this_thread::sleep_for(chrono::microseconds(20));
		}
}

void Worker::terminate()
{
//	cnote << "stopWorking for thread" << m_name;
	ETH_GUARDED(x_work)
		if (m_work)
		{
			cnote << "Terminating" << m_name;
			m_state.exchange(WorkerState::Killing);

			m_work->join();
			m_work.reset();
		}
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
