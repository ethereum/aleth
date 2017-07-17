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
		m_stateRoot = (h256)r[0];
	else if (r[0].size() == 1)
	{
		m_statusCode = (uint8_t)r[0];
		m_hasStatusCode = true;
	}

	m_gasUsed = (u256)r[1];
	m_bloom = (LogBloom)r[2];
	for (auto const& i : r[3])
		m_log.emplace_back(i);

}

TransactionReceipt::TransactionReceipt(h256 const& _root, u256 const& _gasUsed, LogEntries const& _log):
	m_hasStatusCode(false),
	m_stateRoot(_root),
	m_gasUsed(_gasUsed),
	m_bloom(eth::bloom(_log)),
	m_log(_log)
{}

TransactionReceipt::TransactionReceipt(uint8_t _status, u256 const& _gasUsed, LogEntries const& _log):
	m_hasStatusCode(true),
	m_statusCode(_status),
	m_gasUsed(_gasUsed),
	m_bloom(eth::bloom(_log)),
	m_log(_log)
{}

void TransactionReceipt::streamRLP(RLPStream& _s) const
{
	_s.appendList(4);
	if (m_hasStatusCode)
		_s << m_statusCode;
	else
		_s << m_stateRoot;

	_s << m_gasUsed << m_bloom;

	_s.appendList(m_log.size());
	for (LogEntry const& l: m_log)
		l.streamRLP(_s);
}

uint8_t TransactionReceipt::statusCode() const
{
	if (!m_hasStatusCode)
		BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
	return m_statusCode;
}

h256 const& TransactionReceipt::stateRoot() const
{
	if (m_hasStatusCode)
		BOOST_THROW_EXCEPTION(TransactionReceiptVersionError());
	return m_stateRoot;
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
