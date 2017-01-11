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


#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

void VM::reportStackUse()
{
	static intptr_t p = 0;
	intptr_t q = intptr_t(&q);
	if (p)
		cerr << "STACK: " << p << " - " << q << " = " << (p - q) << endl;
	p = q;
}

std::array<InstructionMetric, 256> VM::c_metrics;
void VM::initMetrics()
{
	static bool done=false;
	if (!done)
	{
		for (unsigned i = 0; i < 256; ++i)
		{
			InstructionInfo op = instructionInfo((Instruction)i);
			c_metrics[i].gasPriceTier = op.gasPriceTier;
			c_metrics[i].args = op.args;
			c_metrics[i].ret = op.ret;
		}
	}
	done = true;
}

void VM::copyCode(int _extraBytes)
{
	// Copy code so that it can be safely modified and extend code by
	// _extraBytes zero bytes to allow reading virtual data at the end
	// of the code without bounds checks.
	auto extendedSize = m_ext->code.size() + _extraBytes;
	m_codeSpace.reserve(extendedSize);
	m_codeSpace = m_ext->code;
	m_codeSpace.resize(extendedSize);
	m_code = m_codeSpace.data();
}

void VM::optimize()
{
	size_t pc, nBytes = m_ext->code.size();

	// build a table of jump destinations for use in verifyJumpDest
	
	TRACE_STR(1, "Build JUMPDEST table")
	for (pc = 0; pc < nBytes; ++pc)
	{
		Instruction op = Instruction(m_ext->code[pc]);
		TRACE_OP(2, pc, op);
				
		// make synthetic ops in user code trigger invalid instruction if run
		if (
			op == Instruction::PUSHC ||
			op == Instruction::JUMPC ||
			op == Instruction::JUMPCI
		)
		{
			TRACE_OP(1, pc, op);
			m_ext->code[pc] = (byte)Instruction::BAD;
		}

		if (op == Instruction::JUMPDEST)
		{
			m_jumpDests.push_back(pc);
		}
		else if (
			(byte)Instruction::PUSH1 <= (byte)op &&
			(byte)op <= (byte)Instruction::PUSH32
		)
		{
			pc += (byte)op - (byte)Instruction::PUSH1 + 1;
		}
#ifdef EVM_JUMPS_AND_SUBS
		else if (
			op == Instruction::JUMPTO ||
			op == Instruction::JUMPIF ||
			op == Instruction::JUMPSUB)
		{
			++pc;
			pc += 4;
		}
		else if (op == Instruction::JUMPV || op == Instruction::JUMPSUBV)
		{
			++pc;
			pc += 4 * m_ext->code[pc];  // number of 4-byte dests followed by table
		}
		else if (op == Instruction::BEGINSUB)
		{
			m_beginSubs.push_back(pc);
		}
		else if (op == Instruction::BEGINDATA)
		{
			break;
		}
#endif
	}
	
	copyCode(pc - nBytes);

#ifdef EVM_DO_FIRST_PASS_OPTIMIZATION
	
	#ifdef EVM_USE_CONSTANT_POOL
	
		// maintain constant pool as a hash table of up to 256 u256 constants
		struct hash256
		{
			// FNV chosen as good, fast, and byte-at-a-time
			const uint32_t FNV_PRIME1 = 2166136261;
			const uint32_t FNV_PRIME2 = 16777619;
			uint32_t hash = FNV_PRIME1;
			
			u256 (&table)[256];
			bool empty[256];
			
			hash256(u256 (&table)[256]) : table(table)
			{
				for (int i = 0; i < 256; ++i)
				{
					table[i] = 0;
					empty[i] = true;
				}
			}
			
			void hashInit() { hash = FNV_PRIME1; }

			// hash in successive bytes
			void hashByte(byte b) { hash ^= (b), hash *= FNV_PRIME2; }
		
			// fold hash into 1 byte
			byte getHash() { return ((hash >> 8) ^ hash) & 0xff; }
		
			// insert value at byte index in table, false if collision
			bool insertVal(byte hash, u256& val)
			{
				if (empty[hash])
				{
					empty[hash] = false;
					table[hash] = val;
					return true;
				}
				return table[hash] == val;
			}
		} constantPool(m_pool);
		#define CONST_POOL_HASH_INIT() constantPool.hashInit()
		#define CONST_POOL_HASH_BYTE(b) constantPool.hashByte(b)
		#define CONST_POOL_GET_HASH() constantPool.getHash()
		#define CONST_POOL_INSERT_VAL(hash, val) constantPool.insertVal((hash), (val))
	#else
		#define CONST_POOL_HASH_INIT()
		#define CONST_POOL_HASH_BYTE(b)
		#define CONST_POOL_GET_HASH() 0
		#define CONST_POOL_INSERT_VAL(hash, val) false
	#endif

	TRACE_STR(1, "Do first pass optimizations")
	for (pc = 0; pc < nBytes; ++pc)
	{
		u256 val = 0;
		Instruction op = Instruction(m_code[pc]);

		if ((byte)Instruction::PUSH1 <= (byte)op && (byte)op <= (byte)Instruction::PUSH32)
		{
			byte nPush = (byte)op - (byte)Instruction::PUSH1 + 1;


			// decode pushed bytes to integral value
			CONST_POOL_HASH_INIT();
			val = m_code[pc+1];
			for (uint64_t i = pc+2, n = nPush; --n; ++i) {
				val = (val << 8) | m_code[i];
				CONST_POOL_HASH_BYTE(m_code[i]);
			}

		#ifdef EVM_USE_CONSTANT_POOL
			if (1 < nPush)
			{
				// try to put value in constant pool at hash index
				// if there is no collision replace PUSHn with PUSHC
				TRACE_PRE_OPT(1, pc, op);
				byte hash = CONST_POOL_GET_HASH();
				if (CONST_POOL_INSERT_VAL(hash, val))
				{
					m_code[pc] = (byte)Instruction::PUSHC;
					m_code[pc+1] = hash;
					m_code[pc+2] = nPush - 1;
					TRACE_VAL(1, "constant pooled", val);
				}
				TRACE_POST_OPT(1, pc, op);
			}
		#endif

		#ifdef EVM_REPLACE_CONST_JUMP	
			// replace JUMP or JUMPI to constant location with JUMPC or JUMPCI
			// verifyJumpDest is M = log(number of jump destinations)
			// outer loop is N = number of bytes in code array
			// so complexity is N log M, worst case is N log N
			size_t i = pc + nPush + 1;
			op = Instruction(m_code[i]);
			if (op == Instruction::JUMP)
			{
				TRACE_STR(1, "Replace const JUMPC")
				TRACE_PRE_OPT(1, i, op);
				
				if (0 <= verifyJumpDest(val, false))
					m_code[i] = byte(op = Instruction::JUMPC);
				
				TRACE_POST_OPT(1, i, op);
			}
			else if (op == Instruction::JUMPI)
			{
				TRACE_STR(1, "Replace const JUMPCI")
				TRACE_PRE_OPT(1, i, op);
				
				if (0 <= verifyJumpDest(val, false))
					m_code[i] = byte(op = Instruction::JUMPCI);
				
				TRACE_POST_OPT(1, ii, op);
			}
		#endif

			pc += nPush;
		}
		
	}
	TRACE_STR(1, "Finished optimizations")
#endif	
}


//
// Init interpreter on entry.
//
void VM::initEntry()
{
	m_bounce = &VM::interpretCases; 	
	interpretCases(); // first call initializes jump table
	initMetrics();
	optimize();
}


// Implementation of EXP.
//
// This implements exponentiation by squaring algorithm.
// Is faster than boost::multiprecision::powm() because it avoids explicit
// mod operation.
// Do not inline it.
u256 VM::exp256(u256 _base, u256 _exponent)
{
	using boost::multiprecision::limb_type;
	u256 result = 1;
	while (_exponent)
	{
		if (static_cast<limb_type>(_exponent) & 1)	// If exponent is odd.
			result *= _base;
		_base *= _base;
		_exponent >>= 1;
	}
	return result;
}
