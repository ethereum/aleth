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
#include <libdevcrypto/SHA3.h>
#include <libethcore/BlockInfo.h>
#include <libethcore/Params.h>
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
	virtual void reset(u256 _gas = 0) noexcept override final;

	virtual bytesConstRef go(ExtVMFace& _ext, OnOpFunc const& _onOp = {}, uint64_t _steps = (uint64_t)-1) override final;

	void require(u256 _n) { if (m_stack.size() < _n) { if (m_onFail) m_onFail(); BOOST_THROW_EXCEPTION(StackTooSmall() << RequirementError((bigint)_n, (bigint)m_stack.size())); } }
	void requireMem(unsigned _n) { if (m_temp.size() < _n) { m_temp.resize(_n); } }

	u256 curPC() const { return m_curPC; }

	bytes const& memory() const { return m_temp; }
	u256s const& stack() const { return m_stack; }

private:
	friend class VMFactory;

	/// Construct VM object.
	explicit VM(u256 _gas): VMFace(_gas) {}

	u256 m_curPC = 0;
	bytes m_temp;
	u256s m_stack;
	std::set<u256> m_jumpDests;
	std::function<void()> m_onFail;
};

}
}
