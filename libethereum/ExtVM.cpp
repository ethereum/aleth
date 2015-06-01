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
/** @file ExtVM.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "ExtVM.h"

#include "Executive.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

bool ExtVM::call(CallParameters& _p)
{
	m_callStack.emplace(m_s, lastHashes, depth + 1);
	try
	{
		Executive& e = m_callStack.top();
		if (!e.call(_p, gasPrice, origin))
		{
			e.go(_p.onOp);
			e.accrueSubState(sub);
		}
		_p.gas = e.endGas();
		e.out().copyTo(_p.out);

		m_callStack.pop();
		return !e.excepted();
	}
	catch(...)
	{
		m_callStack.pop();
		throw;
	}
}

h160 ExtVM::create(u256 _endowment, u256& io_gas, bytesConstRef _code, OnOpFunc const& _onOp)
{
	// Increment associated nonce for sender.
	m_s.noteSending(myAddress);

	m_callStack.emplace(m_s, lastHashes, depth + 1);
	try
	{
		Executive& e = m_callStack.top();
		if (!e.create(myAddress, _endowment, gasPrice, io_gas, _code, origin))
		{
			e.go(_onOp);
			e.accrueSubState(sub);
		}
		io_gas = e.endGas();
		m_callStack.pop();
		return e.newAddress();
	}
	catch(...)
	{
		m_callStack.pop();
		throw;
	}
}

