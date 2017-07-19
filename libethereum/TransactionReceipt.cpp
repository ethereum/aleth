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
/** @file TransactionReceipt.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "TransactionReceipt.h"
#include <libethcore/Exceptions.h>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/get.hpp>


using namespace std;
using namespace dev;
using namespace dev::eth;

TransactionReceipt::TransactionReceipt(bytesConstRef _rlp)
{
	RLP r(_rlp);
	if (!r.isList() || r.itemCount() != 4)
		BOOST_THROW_EXCEPTION(InvalidTransactionReceiptFormat());

	if (!r[0].isData())
		BOOST_THROW_EXCEPTION(InvalidTransactionReceiptFormat());
	if (r[0].size() == 32)
		m_statusCodeOrStateRoot = (h256)r[0];
	else if (r[0].size() == 1)
		m_statusCodeOrStateRoot = (uint8_t)r[0];

	m_gasUsed = (u256)r[1];
	m_bloom = (LogBloom)r[2];
	for (auto const& i : r[3])
		m_log.emplace_back(i);

}

TransactionReceipt::TransactionReceipt(h256 const& _root, u256 const& _gasUsed, LogEntries const& _log):
	m_statusCodeOrStateRoot(_root),
	m_gasUsed(_gasUsed),
	m_bloom(eth::bloom(_log)),
	m_log(_log)
{}

TransactionReceipt::TransactionReceipt(uint8_t _status, u256 const& _gasUsed, LogEntries const& _log):
	m_statusCodeOrStateRoot(_status),
	m_gasUsed(_gasUsed),
	m_bloom(eth::bloom(_log)),
	m_log(_log)
{}

void TransactionReceipt::streamRLP(RLPStream& _s) const
{
	struct appenderVisitor : boost::static_visitor<void>
	{
		appenderVisitor(RLPStream& _s) : m_stream(_s) {}
		RLPStream& m_stream;
		void operator()(uint8_t _statusCode) const
		{
			m_stream << _statusCode;
		}
		void operator()(h256 _stateRoot) const
		{
			m_stream << _stateRoot;
		}
	};

	_s.appendList(4);
	boost::apply_visitor(appenderVisitor(_s), m_statusCodeOrStateRoot);
	_s << m_gasUsed << m_bloom;
	_s.appendList(m_log.size());
	for (LogEntry const& l: m_log)
		l.streamRLP(_s);
}

bool TransactionReceipt::hasStatusCode() const
{
	struct hasStatusCodeVisitor : boost::static_visitor<bool>
	{
		bool operator()(uint8_t) const
		{
			return true;
		}
		bool operator()(h256) const
		{
			return false;
		}
	};
	return boost::apply_visitor(hasStatusCodeVisitor(), m_statusCodeOrStateRoot);
}

uint8_t TransactionReceipt::statusCode() const
{
	struct statusCodeVisitor : boost::static_visitor<uint8_t>
	{
		uint8_t operator()(uint8_t _statusCode) const
		{
			return _statusCode;
		}
		uint8_t operator()(h256) const
		{
			BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
		}
	};
	return boost::apply_visitor(statusCodeVisitor(), m_statusCodeOrStateRoot);
}

h256 const& TransactionReceipt::stateRoot() const
{
	try
	{
		return boost::get<h256>(m_statusCodeOrStateRoot);
	}
	catch(const boost::bad_get&)
	{
		BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
	}
}

std::ostream& dev::eth::operator<<(std::ostream& _out, TransactionReceipt const& _r)
{
	if (_r.hasStatusCode())
		_out << "Status: " << _r.statusCode() << std::endl;
	else
		_out << "Root: " << _r.stateRoot() << std::endl;
	_out << "Gas used: " << _r.gasUsed() << std::endl;
	_out << "Logs: " << _r.log().size() << " entries:" << std::endl;
	for (LogEntry const& i: _r.log())
	{
		_out << "Address " << i.address << ". Topics:" << std::endl;
		for (auto const& j: i.topics)
			_out << "  " << j << std::endl;
		_out << "  Data: " << toHex(i.data) << std::endl;
	}
	_out << "Bloom: " << _r.bloom() << std::endl;
	return _out;
}
