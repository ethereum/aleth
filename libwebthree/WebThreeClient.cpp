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
/** @file WebThreeClient.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <memory>
#include "WebThreeClient.h"

using namespace std;
using namespace dev;

WebThreeClient::WebThreeClient(WebThreeServiceType _type): m_io(), m_connection(m_io, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 30310), _type, nullptr, nullptr), m_serviceType(_type)
{
	startClient();
}

WebThreeClient::~WebThreeClient()
{
}

void WebThreeClient::startClient()
{
	m_ioThread = std::thread([&]()
	{
		m_io.run();
	});
}

bytes WebThreeClient::request(NetMsgType _type, RLP const& _request)
{
	promiseResponse p;
	std::future<std::shared_ptr<WebThreeResponse>> f = p.get_future();
	
	NetMsg msg(m_serviceType, nextDataSequence(), _type, _request);
	{
		lock_guard<mutex> l(x_promises);
		m_promises.push_back(make_pair(msg.sequence(),&p));
	}
	
	m_connection.send(msg);
	
	auto s = f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(10));
	if (s != future_status::ready)
	{
		// todo: mutex m_promises and remove promise
		throw WebThreeRequestTimeout();
	}
	
	return std::move(f.get()->payload());
}

NetMsgSequence WebThreeClient::nextDataSequence()
{
	return m_clientSequence++;
}

