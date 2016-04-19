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
/** @file VM.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <unordered_map>
#include <libdevcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libevmcore/Instruction.h>
#include <libdevcore/SHA3.h>
#include <libethcore/BlockHeader.h>
#include "VMFace.h"

namespace dev
{
namespace eth
{

// Convert from a 256-bit integer stack/memory entry into a 160-bit Address hash.
// Currently we just pull out the right (low-order in BE) 160-bits.
inline Address asAddress(u256 _item)
{
	return right160(h256(_item));
}

inline u256 fromAddress(Address _a)
{
	return (u160)_a;
}


/**
 */
class VM: public VMFace
{
public:
	virtual bytesConstRef execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

	uint64_t curPC() const { return m_curPC; }

	bytes const& memory() const { return m_mem; }
	u256s const stack() const { return u256s(); }

	// real machine word, virtual machine word, signed and unsigned overflow words
	typedef uint64_t mw64;
	typedef mw64 rmword;
	typedef u256 vmword;
	typedef s512 soword;
	typedef u512 uoword;
		
	// checked generic convertions
	template<class T> static rmword to_rmword(T v) { if (rmword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return rmword(v); }
	template<class T> static uoword to_uoword(T v) { if (uoword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return uoword(v); }
	template<class T> static soword to_soword(T v) { if (soword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return soword(v); }

private:

	void checkRequirements(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp, Instruction _inst);
	void checkStack(unsigned _n, unsigned _d);
	void requireMem(unsigned _n) { if (m_mem.size() < _n) { m_mem.resize(_n); } }
	uint64_t verifyJumpDest(u256 const& _dest);
	void copyDataToMemory(bytesConstRef _data);
	uint64_t execOrdinaryOpcode(Instruction _inst, u256& io_gas, ExtVMFace& _ext);

	std::unordered_set<uint64_t> m_jumpDests;
	std::function<void()> m_onFail;
	EVMSchedule const* m_schedule = nullptr;
	
	// state of the machine
	Instruction inst;
	uint64_t m_curPC = 0;
	uint64_t m_steps = 0;
	bytes m_mem;
	
	// unlike vector this stack doesn't grow or check bounds - checkStack() does that
	struct stack : std::array<u256,1024> {
		size_t i = -1;
		u256& back() { return (*this)[i]; }
		const u256& back() const { return (*this)[i]; }
		void push_back(const u256& v) {
			(*this)[++i] = v;
		}
		void pop_back() {
			--i;
		}
	   size_t size() const { return i+1; }
		void reserve(size_t) {}
	} m_stack;

	//u256s m_stack;

	// state of the metering and memorizing
	rmword runGas = 0;
	rmword newMemSize = 0;
	rmword copySize = 0;
	
};

}
}
