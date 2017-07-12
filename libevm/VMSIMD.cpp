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
#if EIP_616

namespace dev
{
namespace eth
{

// conversion functions to overlay vectors on storage for u256 stack slots
// a dirty trick but it keeps the SIMD types from polluting the rest of the VM
// so at least assert there's room for the trick, and use wrappers for some safety
static_assert(sizeof(uint64_t[4]) <= sizeof(u256), "stack slot too narrow for SIMD");
using a64x4  = uint64_t[4];		
using a32x8  = uint32_t[8];
using a16x16 = uint16_t[16];
using a8x32  = uint8_t [32];
inline a64x4       & v64x4 (u256      & _stack_item) { return (a64x4&) *(a64x4*) &_stack_item; }
inline a32x8       & v32x8 (u256      & _stack_item) { return (a32x8&) *(a32x8*) &_stack_item; }
inline a16x16      & v16x16(u256      & _stack_item) { return (a16x16&)*(a16x16*)&_stack_item; }
inline a8x32       & v8x32 (u256      & _stack_item) { return (a8x32&) *(a8x32*) &_stack_item; }
inline a64x4  const& v64x4 (u256 const& _stack_item) { return (a64x4&) *(a64x4*) &_stack_item; }
inline a32x8  const& v32x8 (u256 const& _stack_item) { return (a32x8&) *(a32x8*) &_stack_item; }
inline a16x16 const& v16x16(u256 const& _stack_item) { return (a16x16&)*(a16x16*)&_stack_item; }
inline a8x32  const& v8x32 (u256 const& _stack_item) { return (a8x32&) *(a8x32*) &_stack_item; }

class Pow2Bits enum { BITS_8, BITS_16, BITS_32, BITS_64 };

// tried using template template functions, gave up fighting the compiler after a day
#define EVALXOPS(OP, _type) EVALXOP(OP, int8_t, int16_t, int32_t, int64_t, _type)
#define EVALXOPU(OP, _type) EVALXOP(OP, uint8_t, uint16_t, uint32_t, uint64_t, _type)
#define EVALXOP(OP, T8, T16, T32, T64, _type) \
{ \
	uint8_t const t = (_type) & 0xf; \
	m_SPP[0] = 0; \
	switch (t) \
	{ \
	case BITS_8: \
		for (int i = 0; i < 32; ++i) \
			v8x32(m_SPP[0])[i]  = (uint8_t) OP((T8) v8x32(m_SP[0])[i],  (T8) v8x32(m_SP[1])[i]); \
		break; \
	case BITS_16: \
		for (int i = 0; i < 16; ++i) \
			v16x16(m_SPP[0])[i] = (uint16_t)OP((T16)v16x16(m_SP[0])[i], (T16)v16x16(m_SP[1])[i]); \
		break; \
	case BITS_32: \
		for (int i = 0; i < 8; ++i) \
			v32x8(m_SPP[0])[i]  = (uint32_t)OP((T32)v32x8(m_SP[0])[i],  (T32)v32x8(m_SP[1])[i]); \
		break; \
	case BITS_64: \
		for (int i = 0; i < 4; ++i) \
			v64x4(m_SPP[0])[i]  = (uint64_t)OP((T64)v64x4(m_SP[0])[i],  (T64)v64x4(m_SP[1])[i]); \
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
#define NOT( x1, x2) (~(x1))
#define SHR( x1, x2) ((x1) >> (x2))
#define SHL( x1, x2) ((x1) << (x2))
#define ROL( x1, x2) (((x1) << (x2))|((x1) >> (sizeof(x1) * 8 - (x2))))
#define ROR( x1, x2) (((x1) >> (x2))|((x1) << (sizeof(x1) * 8 - (x2))))

void VM::xadd (uint8_t _type) { EVALXOPU(ADD, _type); }
void VM::xmul (uint8_t _type) { EVALXOPU(MUL, _type); }
void VM::xsub (uint8_t _type) { EVALXOPU(SUB, _type); }
void VM::xdiv (uint8_t _type) { EVALXOPU(DIV, _type); }
void VM::xsdiv(uint8_t _type) { EVALXOPS(DIV, _type); }
void VM::xmod (uint8_t _type) { EVALXOPU(MOD, _type); }
void VM::xsmod(uint8_t _type) { EVALXOPS(MOD, _type); }
void VM::xlt  (uint8_t _type) { EVALXOPU(LT,  _type); }
void VM::xslt (uint8_t _type) { EVALXOPS(LT,  _type); }
void VM::xgt  (uint8_t _type) { EVALXOPU(GT,  _type); }
void VM::xsgt (uint8_t _type) { EVALXOPS(GT,  _type); }
void VM::xeq  (uint8_t _type) { EVALXOPU(EQ,  _type); }
void VM::xzero(uint8_t _type) { EVALXOPU(ZERO,_type); }
void VM::xand (uint8_t _type) { EVALXOPU(AND, _type); }
void VM::xoor (uint8_t _type) { EVALXOPU(OR,  _type); }
void VM::xxor (uint8_t _type) { EVALXOPU(XOR, _type); }
void VM::xnot (uint8_t _type) { EVALXOPU(NOT, _type); }
void VM::xshr (uint8_t _type) { EVALXOPU(SHR, _type); }
void VM::xsar (uint8_t _type) { EVALXOPS(SHR, _type); }
void VM::xshl (uint8_t _type) { EVALXOPU(SHL, _type); }
void VM::xrol (uint8_t _type) { EVALXOPU(ROL, _type); }
void VM::xror (uint8_t _type) { EVALXOPU(ROR, _type); }

inline uint8_t pow2N(uint8_t _n)
{
	static uint8_t exp[6] = { 1, 2, 4, 8, 16, 32 };
	return exp[_n];
}

inline uint8_t laneCount(uint8_t _type)
{
	return pow2N((_type) & 0xf);
}

inline uint8_t laneWidth(uint8_t _type)
{
	return (_type) >> 4;
}

// in must be by reference because it is really just memory for a vector
u256 VM::vtow(uint8_t _type, const u256& _in)
{
	u256 out;
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);
	switch (width)
	{
	case BITS_8:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			out << 8;
			out |= v8x32(_in) [i];
		}
		break;
	case BITS_16:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			out << 16;
			out |= v16x16(_in)[i];
		}
		break;
	case BITS_32:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			out << 32;
			out |= v32x8(_in) [i];
		}
		break;
	case BITS_64:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			out << 64;
			out |= v64x4(_in) [i];
		}
		break;
	default:
		throwBadInstruction();
	}
	return out;
}

// out must be by reference because it is really just memory for a vector
void VM::wtov(uint8_t _type, u256 _in, u256& _o_out)
{
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);
	switch (width)
	{
	case BITS_8:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			v8x32(_o_out) [i] = (uint8_t )(_in & 0xff);
			_in >>= 8;
		}
		break;
	case BITS_16:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			v16x16(_o_out)[i] = (uint16_t)(_in & 0xffff);
			_in >>= 16;
		}
		break;
	case BITS_32:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			v32x8(_o_out) [i] = (uint32_t)(_in & 0xffffff);
			_in >>= 32;
		}
		break;
	case BITS_64:
		for (int i = count - 1; 0 <= i; --i)
		{ 
			v64x4(_o_out) [i] = (uint64_t)(_in & 0xffffffff);
			_in >>= 64;
		}
		break;
	default:
		throwBadInstruction();
	}
}

void VM::xmload (uint8_t _type)
{
	// goes onto stack element by element, LSB first
	uint8_t const* p = m_mem.data() + toInt15(m_SP[0]);
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);

	switch (width)
	{
	case BITS_8:
		for (int j = 1,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v |= p[--j];
			v8x32(m_SPP[0])[i] = v;
		}
		break;
	case BITS_16:
		for (int j = 2,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v16x16(m_SPP[0])[i] = v;
		}
		break;
	case BITS_32:
		for (int j = 4,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v32x8(m_SPP[0])[i] = v;
		}
		break;
	case BITS_64:
		for (int j = 8,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v <<= 8;
			v |= p[--j];
			v64x4(m_SPP[0])[i] = v;
		}
		break;
	default:
		throwBadInstruction();
	}
}
   
void VM::xmstore(uint8_t _type)
{
	// n bytes of type t elements in stack vector
	// goes onto memory by element, LSB first
	uint8_t *p = m_mem.data() + toInt15(m_SP[0]);
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);

	switch (width)
	{
	case BITS_8:
		for (int j = 1,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v = v8x32(m_SPP[0])[i];
			p[--j] = (uint8_t)v;
		}
		break;
	case BITS_16:
		for (int j = 2,  i = count - 1; 0 <= i; --i)
		{
			int v = 2;
			v = v16x16(m_SPP[0])[i];
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
		}
		break;
	case BITS_32:
		for (int j = 4,  i = count - 1; 0 <= i; --i)
		{
			int v = 4;
			v = v32x8(m_SPP[0])[i];
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
		}
		break;
	case BITS_64:
		for (int j = 8,  i = count - 1; 0 <= i; --i)
		{
			int v = 0;
			v = v64x4(m_SPP[0])[i];
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
			v >>= 8;
			p[--j] = (uint8_t)v;
		}
		break;
	default:
		throwBadInstruction();
	}
}

void VM::xsload(uint8_t _type)
{
	u256 w = m_ext->store(m_SP[0]);
	wtov(_type, w, m_SPP[0]);
}

void VM::xsstore(uint8_t _type)
{
	u256 w = vtow(_type, m_SP[1]);
	m_ext->setStore(m_SP[0], w);
}

void VM::xvtowide(uint8_t _type)
{
	m_SPP[0] = vtow(_type, m_SP[0]);
}

void VM::xwidetov(uint8_t _type)
{
	wtov(_type, m_SP[0], m_SPP[0]);
}

void VM::xpush(uint8_t _type)
{
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);
	
	// Construct a vector out of n bytes following XPUSH.
	// This requires the code has been copied and extended by 32 zero
	// bytes to handle "out of code" push data here.

	// given the type of the vector
	// mask and shift in the inline bytes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	m_SPP[0] = 0;
	switch (width)
	{
	case BITS_8:
		for (int i = 0; i < count ; ++i)
		{
			v8x32(m_SPP[0])[i] = m_code[++m_PC];
		}
		break;
	case BITS_16:
		for (int i = 0; i < count; ++i)
		{
			uint16_t v = m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v16x16(m_SPP[0])[i] = v;
		}
		break;
	case BITS_32:
		for (int i = 0; i < count; ++i)
		{
			uint32_t v = m_code[m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v32x8(m_SPP[0])[i] = v;
		}
		break;
	case BITS_64:
		for (int i = 0; i < count; ++i)
		{
			uint64_t v = m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v = (v << 8) | m_code[++m_PC];
			v64x4(m_SPP[0])[i] = v;
		}
		break;
	default:
		throwBadInstruction();
	}
}

void VM::xget(uint8_t _src_type, uint8_t _idx_type)
{
	uint8_t const srcWidth = laneWidth(_src_type);
	uint8_t const idxCount = laneCount(_idx_type);
	uint8_t const idxWidth = laneWidth(_idx_type);
	
	// given the type of the source and index
	// for every element of the index get the indexed element from the source
	switch (srcWidth)
	{
	case BITS_8:

		switch (idxWidth)
		{
		case BITS_8:
			for (int i = 0; i < idxCount; ++i)
				v8x32 (m_SPP[1])[i] = v8x32(m_SP[0])[v8x32 (m_SP[1])[i] % idxCount];
			break;
		case BITS_16:
			for (int i = 0; i< idxCount; ++i)
				v16x16(m_SPP[1])[i] = v8x32(m_SP[0])[v16x16(m_SP[1])[i] % idxCount];
			break;
		case BITS_32:
			for (int i = 0; i< idxCount; ++i)
				v32x8 (m_SPP[1])[i] = v8x32(m_SP[0])[v32x8 (m_SP[1])[i] % idxCount];
			break;
		case BITS_64:
			for (int i = 0; i< idxCount; ++i)
				v64x4 (m_SPP[1])[i] = v8x32(m_SP[0])[v64x4 (m_SP[1])[i] % idxCount];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_16:

		switch (idxWidth)
		{
		case BITS_8:
			for (int i = 0; i < idxCount; ++i)
				v8x32 (m_SPP[0])[i] = v16x16(m_SP[1])[v8x32 (m_SP[0])[i] % idxCount];
			break;
		case BITS_16:
			for (int i = 0; i < idxCount; ++i)
				v16x16(m_SPP[0])[i] = v16x16(m_SP[1])[v16x16(m_SP[0])[i] % idxCount];
			break;
		case BITS_32:
			for (int i = 0; i < idxCount; ++i)
				v32x8 (m_SPP[0])[i] = v16x16(m_SP[1])[v32x8 (m_SP[0])[i] % idxCount];
			break;
		case BITS_64:
			for (int i = 0; i < idxCount; ++i)
				v64x4 (m_SPP[0])[i] = v16x16(m_SP[1])[v64x4 (m_SP[0])[i] % idxCount];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_32:

		switch (idxWidth)
		{
		case BITS_8:
			for (int i = 0; i < idxCount; ++i)
				v8x32 (m_SPP[0])[i] = v32x8(m_SP[1])[v8x32 (m_SP[0])[i] % idxCount];
			break;
		case BITS_16:
			for (int i = 0; i < idxCount; ++i)
				v16x16(m_SPP[0])[i] = v32x8(m_SP[1])[v16x16(m_SP[0])[i] % idxCount];
			break;
		case BITS_32:
			for (int i = 0; i < idxCount; ++i)
				v32x8 (m_SPP[0])[i] = v32x8(m_SP[1])[v32x8 (m_SP[0])[i] % idxCount];
			break;
		case BITS_64:
			for (int i = 0; i < idxCount; ++i)
				v64x4 (m_SPP[0])[i] = v32x8(m_SP[1])[v64x4 (m_SP[0])[i] % idxCount];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_64:

		switch (idxWidth)
		{
		case BITS_8:
			for (int i = 0; i < idxCount; ++i)
				v8x32 (m_SPP[0])[i] = v64x4(m_SP[1])[v8x32 (m_SP[0])[i] % idxCount];
			break;
		case BITS_16:
			for (int i = 0; i < idxCount; ++i)
				v16x16(m_SPP[0])[i] = v64x4(m_SP[1])[v16x16(m_SP[0])[i] % idxCount];
			break;
		case BITS_32:
			for (int i = 0; i < idxCount; ++i)
				v32x8 (m_SPP[0])[i] = v64x4(m_SP[1])[v32x8 (m_SP[0])[i] % idxCount];
			break;
		case BITS_64:
			for (int i = 0; i < idxCount; ++i)
				v64x4 (m_SPP[0])[i] = v64x4(m_SP[1])[v64x4 (m_SP[0])[i] % idxCount];
			break;
		default:
			throwBadInstruction();
		}

	default:
		throwBadInstruction();
	}
}

void VM::xput(uint8_t _src_type, uint8_t _dst_type)
{
	uint8_t const srcWidth = laneWidth(_src_type);
	uint8_t const dstCount = laneCount(_dst_type);
	uint8_t const dstWidth = laneWidth(_dst_type);

	// given the type of the source, destination and index
	// for every element of the index put the indexed replacement in the destination                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
	switch (srcWidth)
	{
	case BITS_8:

		switch (dstWidth)
		{
		case BITS_8:
			for (int i = 0; i < dstCount; ++i)
				v8x32 (m_SPP[0])[v8x32(m_SP[1])[i] % 32] = v8x32(m_SP[0])[i];
			break;
		case BITS_16:
			for (int i = 0; i < dstCount; ++i)
				v16x16(m_SPP[0])[v8x32(m_SP[1])[i] % 16] = v8x32(m_SP[0])[i];
			break;
		case BITS_32:
			for (int i = 0; i < dstCount; ++i)
				v32x8 (m_SPP[0])[v8x32(m_SP[1])[i] %  8] = v8x32(m_SP[0])[i];
			break;
		case BITS_64:
			for (int i = 0; i < dstCount; ++i)
				v64x4 (m_SPP[0])[v8x32(m_SP[1])[i] %  4] = v8x32(m_SP[0])[i];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_16:

		switch (dstWidth)
		{
		case BITS_8:
			for (int i = 0; i < dstCount; ++i)
				v8x32 (m_SPP[0])[v16x16(m_SP[1])[i] % 32] = v16x16(m_SP[0])[i];
			break;
		case BITS_16:
			for (int i = 0; i < dstCount; ++i)
				v16x16(m_SPP[0])[v16x16(m_SP[1])[i] % 16] = v16x16(m_SP[0])[i];
			break;
		case BITS_32:
			for (int i = 0; i < dstCount; ++i)
				v32x8(m_SPP[0])[v16x16(m_SP[1])[i] %  8] = v16x16(m_SP[0])[i];
			break;
		case BITS_64:
			for (int i = 0; i < dstCount; ++i)
				v64x4(m_SPP[0])[v16x16(m_SP[1])[i] %  4] = v16x16(m_SP[0])[i];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_32:

		switch (dstWidth)
		{
		case BITS_8:
			for (int i = 0; i < dstCount; ++i)
				v8x32 (m_SPP[0])[v32x8(m_SP[1])[i] % 32] = v32x8(m_SP[0])[i];
			break;
		case BITS_16:
			for (int i = 0; i < dstCount; ++i)
				v16x16(m_SPP[0])[v32x8(m_SP[1])[i] % 16] = v32x8(m_SP[0])[i];
			break;
		case BITS_32:
			for (int i = 0; i < dstCount; ++i)
				v32x8 (m_SPP[0])[v32x8(m_SP[1])[i] %  8] = v32x8(m_SP[0])[i];
			break;
		case BITS_64:
			for (int i = 0; i < dstCount; ++i)
				v64x4 (m_SPP[0])[v32x8(m_SP[1])[i] %  4] = v32x8(m_SP[0])[i];
			break;
		default:
			throwBadInstruction();
		}

	case BITS_64:

		switch (dstWidth)
		{
		case BITS_8:
			for (int i = 0; i < dstCount; ++i)
				v8x32 (m_SPP[0])[v64x4(m_SP[1])[i] % 32] = v64x4(m_SP[0])[i];
			break;
		case BITS_16:
			for (int i = 0; i < dstCount; ++i)
				v16x16(m_SPP[0])[v64x4(m_SP[1])[i] % 16] = v64x4(m_SP[0])[i];
			break;
		case BITS_32:
			for (int i = 0; i < dstCount; ++i)
				v32x8 (m_SPP[0])[v64x4(m_SP[1])[i] %  8] = v64x4(m_SP[0])[i];
			break;
		case BITS_64:
			for (int i = 0; i < dstCount; ++i)
				v64x4 (m_SPP[0])[v64x4(m_SP[1])[i] %  4] = v64x4(m_SP[0])[i];
			break;
		default:
			throwBadInstruction();
		}

	default:
		throwBadInstruction();
	}
}

void VM::xswizzle(uint8_t _type)
{
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);

	// given the type of the source and mask                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	// for every index in the mask copy out the indexed value in the source
	switch (width)
	{
	case BITS_8:
		for (int i = 0; i < count; ++i)
			v8x32 (m_SPP[0])[i] = v8x32(m_SP[1]) [v8x32 (m_SP[0])[i] % count];
		break;
	case BITS_16:
		for (int i = 0; i < count; ++i)
			v16x16(m_SPP[0])[i] = v16x16(m_SP[1])[v16x16(m_SP[0])[i] % count];
		break;
	case BITS_32:
		for (int i = 0; i < count; ++i)
			v32x8 (m_SPP[0])[i] = v32x8(m_SP[1]) [v32x8 (m_SP[0])[i] % count];
		break;
	case BITS_64:
		for (int i = 0; i < count; ++i)
			v64x4 (m_SPP[0])[i] = v64x4(m_SP[1]) [v64x4 (m_SP[0])[i] % count];
		break;
	default:
		throwBadInstruction();
	}
}

void VM::xshuffle(uint8_t _type)
{
	// n type t elements in source and mask vectors
	uint8_t const count = laneCount(_type);
	uint8_t const width = laneWidth(_type);

	// given the type of the sources and mask                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	// for every index in the mask copy out the indexed value in one of the sources
	switch (width)
	{
	case BITS_8:
		for (int i = 0; i < count; ++i)
		{
			int j = v8x32(m_SP[0]) [i];
			v8x32 (m_SPP[0])[i] = j < count ? v8x32(m_SP[1]) [j] : v8x32 (m_SP[2])[(j - count) % count];
		}
		break;
	case BITS_16:
		for (int i = 0; i < count; ++i)
		{
			int j = v16x16(m_SP[0])[i];
			v16x16(m_SPP[0])[i] = j < count ? v16x16(m_SP[1])[j] : v16x16(m_SP[2])[(j - count) % count];
		}
		break;
	case BITS_32:
		for (int i = 0; i < count; ++i)
		{
			int j = v32x8(m_SP[0]) [i];
			v32x8 (m_SPP[0])[i] = j < count ? v32x8(m_SP[1]) [j] : v32x8 (m_SP[2])[(j - count) % count];
		}
		break;
	case BITS_64:
		for (int i = 0; i < count; ++i)
		{
			int j = v64x4(m_SP[0]) [i];
			v64x4 (m_SPP[0])[i] = j < count ? v64x4(m_SP[1]) [j] : v64x4 (m_SP[2])[(j - count) % count];
		}
		break;
	default:
		throwBadInstruction();
	}
}

}}

#endif
