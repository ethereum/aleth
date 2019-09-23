// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


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
	if (m_work)
	{
		WorkerState ex = WorkerState::Stopped;
		m_state.compare_exchange_strong(ex, WorkerState::Starting);
		m_state_notifier.notify_all();
	}
	else
	{
		m_state = WorkerState::Starting;
		m_state_notifier.notify_all();
		m_work.reset(new thread([&]()
		{
			setThreadName(m_name.c_str());
//			cnote << "Thread begins";
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
					cwarn << "Exception thrown in Worker thread: " << _e.what();
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
			m_state_notifier.wait(l);
}

void Worker::stopWorking()
{
	std::unique_lock<Mutex> l(x_work);
	if (m_work)
	{
		WorkerState ex = WorkerState::Started;
		if (!m_state.compare_exchange_strong(ex, WorkerState::Stopping))
			return;
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
		if (m_state.exchange(WorkerState::Killing) == WorkerState::Killing)
			return; // Somebody else is doing this
		l.unlock();
		m_state_notifier.notify_all();
		DEV_TIMED_ABOVE("Terminate worker", 100)
			m_work->join();

		l.lock();
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
