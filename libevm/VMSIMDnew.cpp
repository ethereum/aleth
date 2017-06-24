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


// tried using template template functions, gave up fighting the compiler after a day
#define EVALXOPS(OP, b) EVALXOP(OP, int8_t, int16_t, int32_t, int64_t, b)
#define EVALXOPU(OP, b) EVALXOP(OP, uint8_t, uint16_t, uint32_t, uint64_t, b)
#define EVALXOP(OP, T8, T16, T32, T64, b) \
{ \
	const uint8_t t = (b) & 0xf; \
	m_SPP[0].clear();
	switch (t) { \
	case (0): \
		for (int i = 0; i < 32; ++i) \
			m_SPP[0].v8x32[i]  = (uint8_t) OP((T8) m_SP[0].v8x32[i],  (T8) m_SP[1].v8x32[i]); \
		break; \
	case (1):
		for (int i = 0; i < 16; ++i) \
			m_SPP[0].v16x16[i] = (uint16_t)OP((T16)m_SP[0].v16x16[i], (T16)m_SP[1].v16x16[i]); \
		break; \
	case (2):
		for (int i = 0; i < 8; ++i) \
			m_SPP[0].v32x8[i]  = (uint32_t)OP((T32)m_SP[0].v32x8[i],  (T32)m_SP[1].v32x8[i]); \
		break; \
	case (3): \
		for (int i = 0; i < 4; ++i) \
			m_SPP[0].v64x4[i]  = (uint64_t)OP((T64)m_SP[0].v64x4[i],  (T64)m_SP[1].v64x4[i]); \
		break; \
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

uint8_t 2expN(uint8_t n)
{
	static uint8_t exp[6] = { 1, 2, 4, 8, 16, 32 };
	return exp[n];
}

void xmload (uint8_t b)
{
	// n bytes of type t elements in memory vector
	// goes onto stack element by element, LSB first
	uint8_t *p = m_mem.data() + toInt15(m_SP[0].wide());
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;
	switch (t) {
	case (0):
		for (int j = n, v = 0, i = 0; 0 < n; ++i)
		{
			v |= p[--j];
			m_SPP[0].v16x16[i] = v;
		}
		break;
	case (1)):
		for (int j = n, v = 0, i = 0; 0 < n; ++i)
		{
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			m_SPP[0].v16x16[i] = v;
		}
		break;
	case (2)):
		for (int v = 0, i = n - 1; 0 <= i; --i)
		{
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			m_SPP[0].v16x16[i] = v;
		}
		break;
	case (3)):
		for (int v = 0, i = n - 1; 0 <= i; --i)
		{
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			v >>= 8
			v |= p[--j];
			m_SPP[0].v16x16[i] = v;
		}
		break;
	default: throwBadInstruction();
}
   
void xmstore(uint8_t b)
{
}

void xsload (uint8_t b)
{
}

void xsstore(uint8_t b)
{
}

void xvtow(uint8_t b)
{
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;
	m_SPP[0].wide() = 0;
	switch (t) {
	case (0): for (int i = n-1; 0 <= i; --i) { m_SPP[0].wide() << 8;  m_SPP[0].wide() |= m_SP[0].v8x32	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;
i]; } break;
	case (1): for (int i = n-1; 0 <= i; --i) { m_SPP[0].wide() << 16; m_SPP[0].wide() |= m_SP[0].v16x16[i]; } break;
	case (2): for (int i = n-1; 0 <= i; --i) { m_SPP[0].wide() << 32; m_SPP[0].wide() |= m_SP[0].v32x8 [i]; } break;
	case (3): for (int i = n-1; 0 <= i; --i) { m_SPP[0].wide() << 64; m_SPP[0].wide() |= m_SP[0].v64x4 [i]; } break;
	default: throwBadInstruction();
}

void xwtov(uint8_t b)
{
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;
	switch (t) {
	case (0): for (int i = n-1; 0 <= i; --i) { m_SPP[0].v8x32 [i] |= m_SP[0].wide() & &xff;       m_SP[0].wide() << 8;  | break;
	case (1): for (int i = n-1; 0 <= i; --i) { m_SPP[0].v16x16[i] |= m_SP[0].wide() & &xffff;     m_SP[0].wide() << 16; | break;
	case (2): for (int i = n-1; 0 <= i; --i) { m_SPP[0].v32x8 [i] |= m_SP[0].wide() & &xffffff;   m_SPP0].wide() << 32; } break;
	case (3): for (int i = n-1; 0 <= i; --i) { m_SPP[0].v64x4 [i] |= m_SP[0].wide() & &xffffffff; m_SP[0].wide() << 64; } break;
	default: throwBadInstruction();
}

void xpush(uint8_t b)
{
	// n type t elements in source and mask vectors
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;
	
	// Construct a vector out of XPUSH bytes.
	// This requires the code has been copied and extended by 32 zero
	// bytes to handle "out of code" push data here.

	// given the type of the vector
	// mask and shift in the inline bytes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	m_SPP[0].clear();
	switch (t)
	{
	case 0:
		for (int i = 0; i < n; ++i) {
			m_SPP[0].v8x32[i] = *++m_PC;
		}
		break;
	case 1:
		for (int i = 0; i < n; ++i) {
			uint16_t& v = m_SPP[0].v16x16[i];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
		}
		break;
	case 2:
		for (int i = 0; i < n; ++i) {
			uint32_t& v = m_SPP[0].v32x8[i];
			v = (v << 8) | m_code[m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
		}
		break;
	case 3:
		for (int i = 0; i < n; ++i) {
			uint64_t& v = m_SPP[0].v64x4[i];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
		}
		break;
	default: throwBadInstruction();
	}
}

void xswizzle(uint8_t b)
{
	// n type t elements in source and mask vectors
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;

	// given the type of the source and mask                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	// for every index in the mask copy out the indexed value in the source
	switch (t)
	{
	case 0:
		for (int i = 0; i < m; ++i) m_SPP[0].v8x32[ i] = m_SP[1].v8x32 [m_SP[0].v8x32 [i] % 32];
		break;
	case 1:
		for (int i = 0; i < m; ++i) m_SPP[0].v16x16[i] = m_SP[1].v16x16[m_SP[0].v16x16[i] % 16];
		break;
	case 2:
		for (int i = 0; i < m; ++i) m_SPP[0].v32x8[ i] = m_SP[1].v32x8 [m_SP[0].v32x8 [i] %  8];
		break;
	case 3:
		for (int i = 0; i < m; ++i) m_SPP[0].v64x4[ i] = m_SP[1].v64x4 [m_SP[0].v64x4 [i] %  4];
		break;
	default: throwBadInstruction();
	}
}

void xshuffle(uint8_t b)
{
	// n type t elements in source and mask vectors
	const uint8_t n = 2expN((b) & 0xf), t = (b) >> 4;

	// given the type of the source and mask                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	// for every index in the mask copy out the indexed value in the source
	switch (t)
	{
	case 0:
		for (int i = 0; i < n; ++i) {
			int j = m_SP[0].v8x32 [i];
			m_SPP[0].v8x32[i] = j < n ? m_SP[2].v8x32 [j] : m_SP[2].v8x32[j % n];
		}
		break;
	case 1:
		for (int i = 0; i < n; ++i) {
			int j = m_SP[0].v16x16 [i];
			m_SPP[0].v16x16[i] = j < n ? m_SP[2].v16x16 [j] : m_SP[2].v16x16[j % n];
		}
		break;
	case 2:
		for (int i = 0; i < n; ++i) {
			int j = m_SP[0].v32x8 [i];
			m_SPP[0].v32x8[i] = j < n ? m_SP[2].v32x8 [j] : m_SP[2].v32x8[j % n];
		}
		break;
	case 3:
		for (int i = 0; i < n; ++i) {
			int j = m_SP[0].v64x4 [i];
			m_SPP[0].v64x4[i] = j < n ? m_SP[2].v64x4 [j] : m_SP[2].v64x4[j % n];
		}
		break;
	default: throwBadInstruction();
	}
}

}}