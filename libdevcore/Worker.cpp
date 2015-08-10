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
	Guard l(x_work);
	m_requestedState = WorkerState::Running;
	if (!m_work)
		m_work.reset(new thread([&]()
		{
			setThreadName(m_name.c_str());
			while (m_requestedState != WorkerState::Terminated)
			{
				m_state = WorkerState::Running;

				try
				{
					startedWorking();
					workLoop();
					doneWorking();
				}
				catch (std::exception const& _e)
				{
					clog(WarnChannel) <<
						"Exception thrown in Worker thread: " <<
						boost::current_exception_diagnostic_information();
				}

				m_state = WorkerState::Stopped;

				DEV_TIMED_ABOVE("Worker stopping", 100)
					while (m_requestedState == WorkerState::Stopped)
						this_thread::sleep_for(chrono::milliseconds(20));
			}
		}));
	DEV_TIMED_ABOVE("Start worker", 100)
		while (m_state != WorkerState::Running)
			this_thread::sleep_for(chrono::microseconds(20));
}

void Worker::stopWorking()
{
	DEV_GUARDED(x_work)
		if (m_work)
		{
			m_requestedState = WorkerState::Stopped;

			DEV_TIMED_ABOVE("Stop worker", 100)
				while (m_state != WorkerState::Stopped)
					this_thread::sleep_for(chrono::microseconds(20));
		}
}

void Worker::terminate()
{
	DEV_GUARDED(x_work)
		if (m_work)
		{
			m_requestedState = WorkerState::Terminated;

			DEV_TIMED_ABOVE("Terminate worker", 100)
				m_work->join();

			m_work.reset();
		}
}

void Worker::workLoop()
{
	while (!shouldStop())
	{
		if (m_idleWaitMs)
			this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
		doWork();
	}
}
