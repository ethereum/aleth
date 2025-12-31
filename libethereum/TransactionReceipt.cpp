// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "TransactionReceipt.h"
#include <libethcore/Exceptions.h>

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
	else if (r[0].isInt())
		m_statusCodeOrStateRoot = (uint8_t)r[0];
	else
		BOOST_THROW_EXCEPTION(InvalidTransactionReceiptFormat());

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
	_s.appendList(4);
	if (hasStatusCode())
		_s << statusCode();
	else
		_s << stateRoot();
	_s << m_gasUsed << m_bloom;
	_s.appendList(m_log.size());
	for (LogEntry const& l: m_log)
		l.streamRLP(_s);
}

bool TransactionReceipt::hasStatusCode() const
{
	return m_statusCodeOrStateRoot.which() == 0;
}

uint8_t TransactionReceipt::statusCode() const
{
	if (hasStatusCode())
		return boost::get<uint8_t>(m_statusCodeOrStateRoot);
	else
		BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
}

h256 const& TransactionReceipt::stateRoot() const
{
	if (hasStatusCode())
		BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
	else
		return boost::get<h256>(m_statusCodeOrStateRoot);
}

std::ostream& dev::eth::operator<<(std::ostream& _out, TransactionReceipt const& _r)
{
	if (_r.hasStatusCode())
		_out << "Status: " << _r.statusCode() << std::endl;
	else
		_out << "Root: " << _r.stateRoot() << std::endl;
	_out << "Gas used: " << _r.cumulativeGasUsed() << std::endl;
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
