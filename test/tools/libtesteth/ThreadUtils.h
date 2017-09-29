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

#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace dev
{
namespace test
{

class ThreadPool
{
public:
	ThreadPool(size_t _maxThreads) : m_isJoining(false) {
		m_isJoining = false;
		for (size_t i = 0; i <_maxThreads; i++)
		{
			auto func = [this](){ this->threadFunction();};
			m_threadVector.push_back(new std::thread(func));
		}
	}

	void addTask(std::function<void(void*)> _func, void* _args)
	{
		std::pair<taskFunc, void*> item;
		item.first = _func;
		item.second = _args;
		std::lock_guard<std::mutex> guard(x_taskMutex);
		m_tasks.push(item);
	}

	void joinTasks()
	{
		m_isJoining = true;
		for (size_t i = 0; i < m_threadVector.size(); i++)
		{
			if (m_threadVector.at(i))
			{
				m_threadVector.at(i)->join();
				delete m_threadVector.at(i);
			}
		}
	}

	~ThreadPool()
	{
		joinTasks();
	}
private:
	void threadFunction()
	{
		while(true)
		{
			bool isTask = false;
			task threadTask;
			{
				std::lock_guard<std::mutex> guard(x_taskMutex);
				if (m_tasks.empty() && m_isJoining)
					break;
				if (!m_tasks.empty())
				{
					threadTask = m_tasks.front();
					m_tasks.pop();
					isTask = true;
				}
			}
			if (isTask)
				threadTask.first(threadTask.second);
		}
	}
private:
	typedef std::function<void(void*)> taskFunc;
	typedef std::pair<taskFunc, void*> task;
	std::queue<task> m_tasks;
	std::mutex x_taskMutex;
	std::vector<std::thread*> m_threadVector;
	bool m_isJoining;
};

}}

