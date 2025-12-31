// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Assertion handling.
#pragma once

#include <boost/exception/info.hpp>
#include <iostream>

namespace dev
{

#if defined(_MSC_VER)
#define ETH_FUNC __FUNCSIG__
#elif defined(__GNUC__)
#define ETH_FUNC __PRETTY_FUNCTION__
#else
#define ETH_FUNC __func__
#endif

#define asserts(A) ::dev::assertAux(A, #A, __LINE__, __FILE__, ETH_FUNC)
#define assertsEqual(A, B) ::dev::assertEqualAux(A, B, #A, #B, __LINE__, __FILE__, ETH_FUNC)

inline bool assertAux(bool _a, char const* _aStr, unsigned _line, char const* _file, char const* _func)
{
	if (!_a)
		std::cerr << "Assertion failed:" << _aStr << " [func=" << _func << ", line=" << _line << ", file=" << _file << "]" << std::endl;
	return !_a;
}

template<class A, class B>
inline bool assertEqualAux(A const& _a, B const& _b, char const* _aStr, char const* _bStr, unsigned _line, char const* _file, char const* _func)
{
	bool c = _a == _b;
	if (!c)
	{
		std::cerr << "Assertion failed: " << _aStr << " == " << _bStr << " [func=" << _func << ", line=" << _line << ", file=" << _file << "]" << std::endl;
		std::cerr << "   Fail equality: " << _a << "==" << _b << std::endl;
	}
	return !c;
}

/// Assertion that throws an exception containing the given description if it is not met.
/// Use it as assertThrow(1 == 1, ExceptionType, "Mathematics is wrong.");
/// Do NOT supply an exception object as the second parameter.
#define assertThrow(_condition, _ExceptionType, _description) \
	::dev::assertThrowAux<_ExceptionType>(_condition, _description, __LINE__, __FILE__, ETH_FUNC)

using errinfo_comment = boost::error_info<struct tag_comment, std::string>;

template <class _ExceptionType>
inline void assertThrowAux(
	bool _condition,
	::std::string const& _errorDescription,
	unsigned _line,
	char const* _file,
	char const* _function
)
{
	if (!_condition)
		::boost::throw_exception(
			_ExceptionType() <<
			::dev::errinfo_comment(_errorDescription) <<
			::boost::throw_function(_function) <<
			::boost::throw_file(_file) <<
			::boost::throw_line(_line)
		);
}

template <class _ExceptionType>
inline void assertThrowAux(
	void const* _pointer,
	::std::string const& _errorDescription,
	unsigned _line,
	char const* _file,
	char const* _function
)
{
	assertThrowAux<_ExceptionType>(_pointer != nullptr, _errorDescription, _line, _file, _function);
}

}
