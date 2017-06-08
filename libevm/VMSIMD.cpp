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

namespace dev
{
namespace eth
{

// tried using template template template functions, gave up fighting compiler after a day
#define EVALXOPS(OP, b) EVALXOP(OP, int8_t, int16_t, int32_t, int64_t, b)
#define EVALXOPU(OP, b) EVALXOP(OP, uint8_t, uint16_t, uint32_t, uint64_t, b)
#define EVALXOP(OP, T8, T16, T32, T64, b) \
{ \
	const uint8_t n = (b) & 0xf, t = (b) >> 4; \
	switch (t) { \
	case (0): for (int i = 0; i < n; ++i) m_SPP[0].v8x32[i]  = (uint8_t) OP((T8) m_SP[0].v8x32[i],  (T8) m_SP[1].v8x32[i]);  break; \
	case (1): for (int i = 0; i < n; ++i) m_SPP[0].v16x16[i] = (uint16_t)OP((T16)m_SP[0].v16x16[i], (T16)m_SP[1].v16x16[i]); break; \
	case (2): for (int i = 0; i < n; ++i) m_SPP[0].v32x8[i]  = (uint32_t)OP((T32)m_SP[0].v32x8[i],  (T32)m_SP[1].v32x8[i]);  break; \
	case (3): for (int i = 0; i < n; ++i) m_SPP[0].v64x4[i]  = (uint64_t)OP((T64)m_SP[0].v64x4[i],  (T64)m_SP[1].v64x4[i]);  break; \
	default: throwBadInstruction(); \
	} \
}
#define ADD( x1, x2) ((x1) + (x2))
#define MUL( x1, x2) ((x1) * (x2))
#define SUB( x1, x2) ((x1) - (x2))
#define DIV( x1, x2) ((x1) / (x2))
#define MOD( x1, x2) ((x1) % (x2))
#define LT(  x1, x2) ((x1) < (x2))
#define GT(  x1, x2) ((x1) > (x2))
#define EQ(  x1, x2) ((x1) == (x2))
#define ZERO(x1, x2) ((x1) == 0)
#define AND( x1, x2) ((x1) & (x2))
#define OR(  x1, x2) ((x1) | (x2))
#define XOR( x1, x2) ((x1) ^ (x2))
#define NOT( x1, x2) (~x1)
#define SHR( x1, x2) ((x1) >> (x2))
#define SHL( x1, x2) ((x1) << (x2))
#define ROL( x1, x2) (((x1) << x2)|((x1) >> (sizeof(x1) * 8 - (x2))))
#define ROR( x1, x2) (((x1) >> x2)|((x1) << (sizeof(x1) * 8 - (x2))))

void VM::xadd (uint8_t b) { EVALXOPU(ADD, b); }
void VM::xmul (uint8_t b) { EVALXOPU(MUL, b); }
void VM::xsub (uint8_t b) { EVALXOPU(SUB, b); }
void VM::xdiv (uint8_t b) { EVALXOPU(DIV, b); }
void VM::xsdiv(uint8_t b) { EVALXOPS(DIV, b); }
void VM::xmod (uint8_t b) { EVALXOPU(MOD, b); }
void VM::xsmod(uint8_t b) { EVALXOPS(MOD, b); }
void VM::xlt  (uint8_t b) { EVALXOPU(LT,  b); }
void VM::xslt (uint8_t b) { EVALXOPS(LT,  b); }
void VM::xgt  (uint8_t b) { EVALXOPU(GT,  b); }
void VM::xsgt (uint8_t b) { EVALXOPS(GT,  b); }
void VM::xeq  (uint8_t b) { EVALXOPU(EQ,  b); }
void VM::xzero(uint8_t b) { EVALXOPU(ZERO,b); }
void VM::xand (uint8_t b) { EVALXOPU(AND, b); }
void VM::xoor (uint8_t b) { EVALXOPU(OR,  b); }
void VM::xxor (uint8_t b) { EVALXOPU(XOR, b); }
void VM::xnot (uint8_t b) { EVALXOPU(NOT, b); }
void VM::xshr (uint8_t b) { EVALXOPU(SHR, b); }
void VM::xsar (uint8_t b) { EVALXOPS(SHR, b); }
void VM::xshl (uint8_t b) { EVALXOPU(SHL, b); }
void VM::xrol (uint8_t b) { EVALXOPU(ROL, b); }
void VM::xror (uint8_t b) { EVALXOPU(ROR, b); }

}}