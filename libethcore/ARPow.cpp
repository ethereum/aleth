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
/** @file ARPow.cpp
 * @author Tim Hughes <tim@twistedfury.com>
 * @date 2014
 */

#if !ETH_LANGUAGES

#include <boost/detail/endian.hpp>
#include <chrono>
#include <array>
#include <random>
#include <thread>
#include <libethcore/CryptoHeaders.h>
#include <libdevcore/Common.h>
#include "ARPow.h"
#include "State.h"
using namespace std;
using namespace std::chrono;

namespace dev {
namespace eth {

u256 ARPowEngine::m_nonce = u256(0);

static inline h256 sha3(h256 const& v)
{
	h256 ret;
	CryptoPP::SHA3_256().CalculateTruncatedDigest(ret.data(), 32, v.data(), 32);
	return ret;
}

static inline h256 sha3(h256 const& a, h256 const& b)
{
	CryptoPP::SHA3_256 sha;
	sha.Update(a.data(), sizeof(a));
	sha.Update(b.data(), sizeof(b));
	h256 ret;
	sha.TruncatedFinal(ret.data(), sizeof(ret));
	return ret;
}

static inline h256 sha3(byte const* data, size_t size)
{
	h256 ret;
	CryptoPP::SHA3_256().CalculateTruncatedDigest(ret.data(), 32, data, size);
	return ret;
}

class SeedObj
{
public:
	SeedObj(u256 const& seed)
		:	m_seed(seed) {}

	unsigned rand(unsigned r)
	{
		u256 quotient = m_seed / r;
		if ((quotient >> 64).is_zero())
		{
			m_seed = (u256)sha3((h256)m_seed);
		}
		else
		{
			m_seed = quotient;
		}
		return (unsigned)(m_seed - quotient*r);
	}

	uint64_t rand()
	{
		uint64_t ret = (uint64_t)m_seed;
		m_seed >>= 64;
		if ((m_seed >> 64).is_zero())
		{
			m_seed = (u256)sha3((h256)m_seed);
		}
		return ret;
	}

private:
	u256 m_seed;
};

ARPowEngine::ARPowEngine()
{
	m_tape = new TapeInstr[c_tapeSize];
	m_mem = new uint64_t[c_memSize];
}

ARPowEngine::~ARPowEngine()
{
	delete [] m_tape;
	delete [] m_mem;
}

void ARPowEngine::genTape(u256 const& nonce)
{
    SeedObj s((u256)sha3(m_root, nonce / c_genInterval));

	for (unsigned i = 0; i != c_tapeSize; ++i)
	{	
        m_tape[i].op = (TapeOp)s.rand(TapeOp_Count);
        unsigned r = s.rand(100u);
		m_tape[i].y = r < 20 && i > 20 ? m_tape[i - 1 - r].x : s.rand(c_memSize);
		m_tape[i].x = s.rand(c_memSize);
	}
}

h256 ARPowEngine::runTape(u256 const& nonce)
{
	SeedObj s((u256)sha3(nonce % c_genInterval));

    for (unsigned i = 0; i != c_memSize; ++i)
	{
		m_mem[i] = s.rand();
	}
    
	// run the tape on the inputs
	int dir = 1;
    for (unsigned i = 0; i != c_tapeSize/100; ++i)
	{
        // 16% of the time, we flip the order of the next 100 ops; this adds in conditional branching
		unsigned start = dir > 0 ? 0 : 100;
		unsigned end = 100 - start;
		unsigned base = i*100 - (dir > 0 ? 0 : 1);

		uint64_t x, y;
        for (unsigned j = start; j != end; j += dir)
		{
            TapeInstr const& instr = m_tape[base + j];

			x = m_mem[instr.x];
			y = m_mem[instr.y];
			switch (instr.op)
			{
			case TapeOp_Add:					x += y; break;
			case TapeOp_Mul:					x *= y; break;
			case TapeOp_Mod:					x %= y; break;
			case TapeOp_Xor:					x ^= y; break;
			case TapeOp_And:					x &= y; break;
			case TapeOp_Or:						x |= y; break;
			case TapeOp_Minus1MinusX:			x = -1-x; break;
			case TapeOp_Xor_Minus1MinusX_Y:		x = (-1-x) ^ y; break;
			case TapeOp_ShiftRight_X_YMod64:	x >>= y; break;
			}
			m_mem[instr.x] = x;
		}
        if ( (unsigned(x % 37) - 2u) < 7u )
		{
			dir = -dir;
		}
    }

	// todo, big endian
	return sha3((byte const*)m_mem, sizeof(m_mem[0]) * c_memSize);
}

bool ARPowEngine::runTapeAndTest(State const& state, u256 const& nonce, u256 const& target)
{
	h256 h = runTape( nonce);
	h160 j = state.nextActiveAddress(right160(h));

	CryptoPP::SHA3_256 sha;
	sha.Update(h.data(), sizeof(h));
	sha.Update(j.data(), sizeof(j));
	h256 hash; sha.TruncatedFinal(hash.data(), sizeof(hash)); 

	return (u256)hash < target;
}

bool ARPowEngine::verify(State const& state, h256 const& root, h256 const& nonce, u256 const& difficulty)
{
	u256 target = (u256)((bigint(1) << 256) / difficulty);
	ARPowEngine instance;
	instance.m_root = root;
	instance.genTape(nonce);
	return instance.runTapeAndTest(state, (u256)nonce, target);
}

MineInfo ARPowEngine::mine(h256& o_solution, State const& state, h256 const& root, u256 const& difficulty, unsigned msTimeout, bool turbo)
{
	MineInfo ret;

	bigint d = (bigint(1) << 256) / difficulty;
	ret.requirement = log2((double)d);

	auto startTime = std::chrono::steady_clock::now();
	auto endtime = startTime + std::chrono::milliseconds(msTimeout);
	if (!turbo)
		std::this_thread::sleep_for(std::chrono::milliseconds(msTimeout * 90 / 100));

	// ideally would resume if root hasn't changed, but need to force genTape here
	m_root = root;
	m_nonce += (c_genInterval - 1);
	m_nonce %= c_genInterval;

	u256 target = (u256)d;
	for (; std::chrono::steady_clock::now() < endtime; ++m_nonce)
	{
		if ((m_nonce % c_genInterval).is_zero())
		{
			genTape(m_nonce);
		}

		// todo: ret.best. 
		// why is this a double?

		if (runTapeAndTest(state, m_nonce, target))
		{
			o_solution = m_nonce;
			ret.completed = true;
			break;
		}
	}

	assert(!ret.completed || verify(state, root, o_solution, difficulty));

	return ret;
}


}
}
#endif
