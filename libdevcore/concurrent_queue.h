// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Exceptions.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace dev
{
/// Concurrent queue.
/// You can push and pop elements to/from the queue. Pop will block until the queue is not empty.
/// The default backend (_QueueT) is std::queue. It can be changed to any type that has
/// proper push(), pop(), empty() and front() methods.
template <typename _T, typename _QueueT = std::queue<_T>>
class concurrent_queue
{
public:
    template <typename _U>
    void push(_U&& _elem)
    {
        {
            std::lock_guard<decltype(x_mutex)> guard{x_mutex};
            m_queue.push(std::forward<_U>(_elem));
        }
        m_cv.notify_one();
    }

	_T pop()
	{
		std::unique_lock<std::mutex> lock{ x_mutex };
		m_cv.wait(lock, [this] { return !m_queue.empty(); });
		auto item = std::move(m_queue.front());
		m_queue.pop();
		return item;
	}

    _T pop(std::chrono::milliseconds const& _waitDuration)
    {
        std::unique_lock<std::mutex> lock{x_mutex};
        if (!m_cv.wait_for(lock, _waitDuration, [this] { return !m_queue.empty(); }))
            BOOST_THROW_EXCEPTION(WaitTimeout());

        auto item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

private:
    _QueueT m_queue;
    std::mutex x_mutex;
    std::condition_variable m_cv;
};

}  // namespace dev
