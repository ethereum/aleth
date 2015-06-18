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
/** @file Common.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Very common stuff (i.e. that every other header needs except vector_ref.h).
 */

#pragma once

// way to many unsigned to size_t warnings in 32 bit build
#ifdef _M_IX86
#pragma warning(disable:4244)
#endif

#ifdef _MSC_VER
#define _ALLOW_KEYWORD_MACROS
#define noexcept throw()
#endif

#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <unordered_set>
#include <functional>
#include <string>
#include <boost/timer.hpp>
#include <boost/functional/hash.hpp>
#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/version.hpp>
#if (BOOST_VERSION == 105800)
	#include "boost_multiprecision_number_compare_bug_workaround.hpp"
#endif
#include <boost/multiprecision/cpp_int.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "vector_ref.h"

// CryptoPP defines byte in the global namespace, so must we.
using byte = uint8_t;

// Quote a given token stream to turn it into a string.
#define DEV_QUOTED_HELPER(s) #s
#define DEV_QUOTED(s) DEV_QUOTED_HELPER(s)

#define DEV_IGNORE_EXCEPTIONS(X) try { X; } catch (...) {}

namespace dev
{

extern char const* Version;

static const std::string EmptyString;

// Binary data types.
using bytes = std::vector<byte>;
using bytesRef = vector_ref<byte>;
using bytesConstRef = vector_ref<byte const>;

// Numeric types.
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;
using u64 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<64, 64, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using u128 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using u256 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using s256 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>;
using u160 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using s160 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>;
using u512 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<512, 512, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using u256s = std::vector<u256>;
using u160s = std::vector<u160>;
using u256Set = std::set<u256>;
using u160Set = std::set<u160>;

extern const u256 UndefinedU256;

// Map types.
using StringMap = std::map<std::string, std::string>;
using BytesMap = std::map<bytes, bytes>;
using u256Map = std::map<u256, u256>;
using HexMap = std::map<bytes, bytes>;

// Hash types.
using StringHashMap = std::unordered_map<std::string, std::string>;
using u256HashMap = std::unordered_map<u256, u256>;

// String types.
using strings = std::vector<std::string>;

// Fixed-length string types.
using string32 = std::array<char, 32>;
static const string32 ZeroString32 = {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }};

// Null/Invalid values for convenience.
static const u256 Invalid256 = ~(u256)0;
static const bytes NullBytes;
static const std::map<u256, u256> EmptyMapU256U256;

/// Interprets @a _u as a two's complement signed number and returns the resulting s256.
inline s256 u2s(u256 _u)
{
	static const bigint c_end = bigint(1) << 256;
	if (boost::multiprecision::bit_test(_u, 255))
		return s256(-(c_end - _u));
	else
		return s256(_u);
}

/// @returns the two's complement signed representation of the signed number _u.
inline u256 s2u(s256 _u)
{
	static const bigint c_end = bigint(1) << 256;
    if (_u >= 0)
		return u256(_u);
    else
		return u256(c_end + _u);
}

/// @returns the smallest n >= 0 such that (1 << n) >= _x
inline unsigned int toLog2(u256 _x)
{
	unsigned ret;
	for (ret = 0; _x >>= 1; ++ret) {}
	return ret;
}

/// @returns the absolute distance between _a and _b.
template <class N>
inline N diff(N const& _a, N const& _b)
{
	return std::max(_a, _b) - std::min(_a, _b);
}

/// RAII utility class whose destructor calls a given function.
class ScopeGuard
{
public:
	ScopeGuard(std::function<void(void)> _f): m_f(_f) {}
	~ScopeGuard() { m_f(); }

private:
	std::function<void(void)> m_f;
};

/// Inheritable for classes that have invariants.
class HasInvariants
{
public:
	/// Check invariants are met, throw if not.
	void checkInvariants() const;

protected:
	/// Reimplement to specify the invariants.
	virtual bool invariants() const = 0;
};

/// RAII checker for invariant assertions.
class InvariantChecker
{
public:
	InvariantChecker(HasInvariants* _this): m_this(_this) { m_this->checkInvariants(); }
	~InvariantChecker() { m_this->checkInvariants(); }

private:
	HasInvariants const* m_this;
};

/// Scope guard for invariant check in a class derived from HasInvariants.
#if ETH_DEBUG
#define DEV_INVARIANT_CHECK { ::dev::InvariantChecker __dev_invariantCheck(this); }
#else
#define DEV_INVARIANT_CHECK (void)0;
#endif

/// Simple scope-based timer helper.
class TimerHelper
{
public:
	TimerHelper(char const* _id, unsigned _msReportWhenGreater = 0): m_id(_id), m_ms(_msReportWhenGreater) {}
	~TimerHelper();

private:
	boost::timer m_t;
	char const* m_id;
	unsigned m_ms;
};

#define DEV_TIMED(S) for (::std::pair<::dev::TimerHelper, bool> __eth_t(#S, true); __eth_t.second; __eth_t.second = false)
#define DEV_TIMED_SCOPE(S) ::dev::TimerHelper __eth_t(S)
#if WIN32
#define DEV_TIMED_FUNCTION DEV_TIMED_SCOPE(__FUNCSIG__)
#else
#define DEV_TIMED_FUNCTION DEV_TIMED_SCOPE(__PRETTY_FUNCTION__)
#endif

#define DEV_TIMED_ABOVE(S, MS) for (::std::pair<::dev::TimerHelper, bool> __eth_t(::dev::TimerHelper(#S, MS), true); __eth_t.second; __eth_t.second = false)
#define DEV_TIMED_SCOPE_ABOVE(S, MS) ::dev::TimerHelper __eth_t(S, MS)
#if WIN32
#define DEV_TIMED_FUNCTION_ABOVE(MS) DEV_TIMED_SCOPE_ABOVE(__FUNCSIG__, MS)
#else
#define DEV_TIMED_FUNCTION_ABOVE(MS) DEV_TIMED_SCOPE_ABOVE(__PRETTY_FUNCTION__, MS)
#endif

enum class WithExisting: int
{
	Trust = 0,
	Verify,
	Kill
};

}

namespace std
{

inline dev::WithExisting max(dev::WithExisting _a, dev::WithExisting _b)
{
	return static_cast<dev::WithExisting>(max(static_cast<int>(_a), static_cast<int>(_b)));
}

template <> struct hash<dev::u256>
{
	size_t operator()(dev::u256 const& _a) const
	{
		unsigned size = _a.backend().size();
		auto limbs = _a.backend().limbs();
		return boost::hash_range(limbs, limbs + size);
	}
};

}
