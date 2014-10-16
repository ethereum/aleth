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
/** @file ARPow.h
 * @author Tim Hughes <tim@twistedfury.com>
 * @date 2014
 */

#pragma once

#include "ProofOfWork.h"

namespace dev {
namespace eth {

class State;

class ARPowEngine
{
public:
	static const unsigned c_genInterval = 1000;
	static const unsigned c_tapeSize = 150000;
	static const unsigned c_memSize = 10000;

public:
	ARPowEngine();
	~ARPowEngine();

	static bool verify(State const& state, h256 const& root, h256 const& nonce, u256 const& difficulty);

	MineInfo mine(h256& o_solution, State const& _state, h256 const& _root, u256 const& _difficulty, unsigned _msTimeout = 100, bool _turbo = false);

private:
	void genTape(u256 const& nonce);
	h256 runTape(u256 const& nonce);
	bool runTapeAndTest(State const& state, u256 const& nonce, u256 const& target);

private:
	enum TapeOp
	{
		TapeOp_Add,
		TapeOp_Mul,
		TapeOp_Mod,
		TapeOp_Xor,
		TapeOp_And,
		TapeOp_Or,
		TapeOp_Minus1MinusX,
		TapeOp_Xor_Minus1MinusX_Y,
		TapeOp_ShiftRight_X_YMod64,
		TapeOp_Count
	};

	struct TapeInstr
	{
		TapeOp op;
		unsigned x;
		unsigned y;
	};

	// should be a member variable, but class isn't persisted between calls
	static u256 m_nonce; 
	h256 m_root;
	TapeInstr* m_tape;
	uint64_t* m_mem;
};

}
}
