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

template<size_t _Size>
class ContextStack
{
public:
	ContextStack(): m_mem(new char[_Size]) {}

	void* ptr() const { return m_mem.get() + _Size; } // stack grows down
	size_t size() const { return _Size; }

	ContextStack(ContextStack const&) = delete;
	ContextStack& operator=(ContextStack) = delete;

private:
	std::unique_ptr<char[]> m_mem;
};

namespace
{
void callProcedure(intptr_t _param)
{
	auto& params = *reinterpret_cast<CallParameters*>(_param);
	auto& e = *params.executive;
	auto& extVM = *params.extVM;
	try
	{
		if (!e.call(params, extVM.gasPrice, extVM.origin))
		{
			e.go(params.onOp);
			e.accrueSubState(extVM.sub);
		}
		params.gas = e.gas();
		e.out().copyTo(params.out);
	}
	catch(...)
	{
		// TODO: Exception can be passed to other context but not by throw.
		//       However Executive should handle its exceptions internally.
		cwarn << "Unexpected exception in CALL.";
	}

	boost::context::jump_fcontext(params.callCtx, params.retCtx, !e.excepted());
}

void createProcedure(intptr_t _param)
{
	auto& params = *reinterpret_cast<CallParameters*>(_param);
	auto& e = *params.executive;
	auto& extVM = *params.extVM;
	try
	{
		if (!e.create(extVM.myAddress, params.value, extVM.gasPrice, params.gas, params.data, extVM.origin))
		{
			e.go(params.onOp);
			e.accrueSubState(extVM.sub);
		}
	}
	catch(...)
	{
		// TODO: Exception can be passed to other context but not by throw.
		//       However Executive should handle its exceptions internally.
		cwarn << "Unexpected exception in CREATE.";
	}

	boost::context::jump_fcontext(params.callCtx, params.retCtx, !e.excepted());
}
}

static const size_t c_contextStackSize = 16 * 1024;

bool ExtVM::call(CallParameters& _p)
{
	Executive e(m_s, lastHashes, depth + 1);
	_p.executive = &e;
	_p.extVM = this;

	boost::context::fcontext_t retCtx;
	_p.retCtx = &retCtx;
	ContextStack<c_contextStackSize> stack;
	_p.callCtx = boost::context::make_fcontext(stack.ptr(), stack.size(), callProcedure);

	auto ret = boost::context::jump_fcontext(_p.retCtx, _p.callCtx, reinterpret_cast<intptr_t>(&_p));

	return static_cast<bool>(ret);
}

h160 ExtVM::create(u256 _endowment, u256& io_gas, bytesConstRef _code, OnOpFunc const& _onOp)
{
	// Increment associated nonce for sender.
	m_s.noteSending(myAddress);

	Executive e(m_s, lastHashes, depth + 1);

	CallParameters params;
	params.value = _endowment;
	params.gas = io_gas;
	params.data = _code;
	params.onOp = _onOp;
	params.executive = &e;
	params.extVM = this;

	boost::context::fcontext_t retCtx;
	params.retCtx = &retCtx;
	ContextStack<c_contextStackSize> stack;
	params.callCtx = boost::context::make_fcontext(stack.ptr(), stack.size(), createProcedure);

	boost::context::jump_fcontext(params.retCtx, params.callCtx, reinterpret_cast<intptr_t>(&params));

	io_gas = e.gas();
	return e.newAddress();
}

