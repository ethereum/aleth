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
/** @file LogFilter.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "LogFilter.h"

#include <libdevcrypto/SHA3.h>
#include "State.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

std::ostream& dev::eth::operator<<(std::ostream& _out, LogFilter const& _s)
{
	// TODO
	_out << "(@" << _s.m_addresses << "#" << _s.m_topics << ">" << _s.m_earliest << "-" << _s.m_latest << "< )";
	return _out;
}

void LogFilter::streamRLP(RLPStream& _s) const
{
	_s.appendList(4) << m_addresses << m_topics << m_earliest << m_latest;
}

h256 LogFilter::sha3() const
{
	RLPStream s;
	streamRLP(s);
	return dev::sha3(s.out());
}

static bool isNoLater(RelativeBlock _logBlockRelation, u256 _logBlockNumber, unsigned _latest)
{
	if (_latest == PendingBlock)
		return true;
	else if (_latest == LatestBlock)
		return _logBlockRelation == RelativeBlock::Latest;
	else
		return _logBlockNumber <= _latest;
}

static bool isNoEarlier(RelativeBlock _logBlockRelation, u256 _logBlockNumber, unsigned _earliest)
{
	if (_earliest == PendingBlock)
		return _logBlockRelation == RelativeBlock::Pending;
	else if (_earliest == LatestBlock)
		return true;
	else
		return _logBlockNumber >= _earliest;
}

bool LogFilter::envelops(RelativeBlock _logBlockRelation, u256 _logBlockNumber) const
{
	return
		isNoLater(_logBlockRelation, _logBlockNumber, m_latest) &&
		isNoEarlier(_logBlockRelation, _logBlockNumber, m_earliest);
}

bool LogFilter::matches(LogBloom _bloom) const
{
	if (m_addresses.size())
	{
		for (auto const& i: m_addresses)
			if (_bloom.containsBloom<3>(dev::sha3(i)))
				goto OK1;
		return false;
	}
	OK1:
	for (auto const& t: m_topics)
		if (t.size())
		{
			for (auto const& i: t)
				if (_bloom.containsBloom<3>(dev::sha3(i)))
					goto OK2;
			return false;
			OK2:;
		}
	return true;
}

bool LogFilter::matches(State const& _s, unsigned _i) const
{
	return matches(_s.receipt(_i)).size() > 0;
}

vector<LogBloom> LogFilter::bloomPossibilities() const
{
	// return combination of each of the addresses/topics
	vector<LogBloom> ret;
	// TODO proper combinatorics.
	for (auto i: m_addresses)
		ret.push_back(LogBloom().shiftBloom<3>(dev::sha3(i)));
	return ret;
}

LogEntries LogFilter::matches(TransactionReceipt const& _m) const
{
	LogEntries ret;
	if (matches(_m.bloom()))
		for (LogEntry const& e: _m.log())
		{
			if (!m_addresses.empty() && !m_addresses.count(e.address))
				goto continue2;
			for (unsigned i = 0; i < 4; ++i)
				if (!m_topics[i].empty() && (e.topics.size() < i || !m_topics[i].count(e.topics[i])))
					goto continue2;
			ret.push_back(e);
			continue2:;
		}
	return ret;
}
