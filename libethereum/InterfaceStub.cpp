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
/** @file InterfaceStub.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <libdevcore/StructuredLogger.h>
#include "InterfaceStub.h"
#include "BlockChain.h"
#include "Executive.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

void InterfaceStub::transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
//	startWorking();
	
	u256 n;
	n = postMine().transactionsFrom(toAddress(_secret));
	Transaction t(_value, _gasPrice, _gas, _dest, _data, n, _secret);
	StructuredLogger::transactionReceived(t.sha3().abridged(), t.sender().abridged());
	cnote << "New transaction " << t;
	m_tq.attemptImport(t.rlp());
}

Address InterfaceStub::transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
//	startWorking();
	
	u256 n;
	n = postMine().transactionsFrom(toAddress(_secret));
	Transaction t(_endowment, _gasPrice, _gas, _init, n, _secret);
	cnote << "New transaction " << t;
	m_tq.attemptImport(t.rlp());
	return right160(sha3(rlpList(t.sender(), t.nonce())));
}


void InterfaceStub::inject(bytesConstRef _rlp)
{
//	startWorking();
	
	m_tq.attemptImport(_rlp);
}


void InterfaceStub::flushTransactions()
{
//	doWork();
}


bytes InterfaceStub::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, int _blockNumber)
{
	bytes out;
	try
	{
		State temp = asOf(_blockNumber);
		u256 n = temp.transactionsFrom(toAddress(_secret));
		Transaction t(_value, _gasPrice, _gas, _dest, _data, n, _secret);
		u256 gasUsed = temp.execute(bc(), t.rlp(), &out, false);
		(void)gasUsed; // TODO: do something with gasused which it returns.
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return out;
}

bytes InterfaceStub::call(Address _dest, bytes const& _data, u256 _gas, u256 _value, u256 _gasPrice)
{
	try
	{
		State temp = postMine();
		Executive e(temp, LastHashes(), 0);
		if (!e.call(_dest, _dest, Address(), _value, _gasPrice, &_data, _gas, Address()))
		{
			e.go();
			return e.out().toBytes();
		}
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return bytes();
}

u256 InterfaceStub::balanceAt(Address _a, int _block) const
{
	return asOf(_block).balance(_a);
}

u256 InterfaceStub::countAt(Address _a, int _block) const
{
	return asOf(_block).transactionsFrom(_a);
}

u256 InterfaceStub::stateAt(Address _a, u256 _l, int _block) const
{
	return asOf(_block).storage(_a, _l);
}

bytes InterfaceStub::codeAt(Address _a, int _block) const
{
	return asOf(_block).code(_a);
}

map<u256, u256> InterfaceStub::storageAt(Address _a, int _block) const
{
	return asOf(_block).storage(_a);
}

h256 InterfaceStub::hashFromNumber(unsigned _number) const
{
	return bc().numberHash(_number);
}

BlockInfo InterfaceStub::blockInfo(h256 _hash) const
{
	return BlockInfo(bc().block(_hash));
}

BlockDetails InterfaceStub::blockDetails(h256 _hash) const
{
	return bc().details(_hash);
}

Transaction InterfaceStub::transaction(h256 _transactionHash) const
{
	return Transaction(bc().transaction(_transactionHash), CheckSignature::Range);
}

Transaction InterfaceStub::transaction(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[1].itemCount())
		return Transaction(b[1][_i].data(), CheckSignature::Range);
	else
		return Transaction();
}

Transactions InterfaceStub::transactions(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	Transactions res;
	for (unsigned i = 0; i < b[1].itemCount(); i++)
		res.emplace_back(b[1][i].data(), CheckSignature::Range);
	return res;
}

TransactionHashes InterfaceStub::transactionHashes(h256 _blockHash) const
{
	return bc().transactionHashes(_blockHash);
}

BlockInfo InterfaceStub::uncle(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[2].itemCount())
		return BlockInfo::fromHeader(b[2][_i].data());
	else
		return BlockInfo();
}

unsigned InterfaceStub::transactionCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[1].itemCount();
}

unsigned InterfaceStub::uncleCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[2].itemCount();
}

unsigned InterfaceStub::number() const
{
	return bc().number();
}

Transactions InterfaceStub::pending() const
{
	return postMine().pending();
}

Addresses InterfaceStub::addresses(int _block) const
{
	Addresses ret;
	for (auto const& i: asOf(_block).addresses())
		ret.push_back(i.first);
	return ret;
}

u256 InterfaceStub::gasLimitRemaining() const
{
	return postMine().gasLimitRemaining();
}


void InterfaceStub::setAddress(Address _us)
{
	preMine().setAddress(_us);
}

Address InterfaceStub::address() const
{
	return preMine().address();
}




