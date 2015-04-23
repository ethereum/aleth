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
/** Abi.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <ostream>
#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include "../test/JsonSpiritHeaders.h"

namespace dev
{
namespace abi
{

struct ExpectedAdditionalParameter: public Exception {};
struct ExpectedOpen: public Exception {};
struct ExpectedClose: public Exception {};

struct UnknownMethod: public Exception {};
struct OverloadedMethod: public Exception {};

enum class Mode
{
	Encode,
	Decode
};

enum class Encoding
{
	Auto,
	Decimal,
	Hex,
	Binary,
};

enum class Tristate
{
	False = false,
	True = true,
	Mu
};

struct EncodingPrefs
{
	Encoding e = Encoding::Auto;
	bool prefix = true;
};

enum class Format
{
	Binary,
	Hex,
	Decimal,
	Open,
	Close
};

enum class Base
{
	Unknown,
	Bytes,
	Address,
	Int,
	Uint,
	Fixed
};

struct ABIType
{
	Base base = Base::Unknown;
	unsigned size = 32;
	unsigned ssize = 0;
	std::vector<int> dims;
	std::string name;
	ABIType() = default;
	ABIType(std::string const& _type, std::string const& _name);

	ABIType(std::string const& _s);

	std::string canon() const;
	bool isBytes() const;

	std::string render(bytes const& _data, EncodingPrefs _e) const;

	bytes unrender(bytes const& _data, Format _f) const;

	void noteHexInput(unsigned _nibbles);
	void noteBinaryInput();
	void noteDecimalInput();

	bytes aligned(bytes const& _b, Format _f, unsigned _length) const;
};

std::tuple<bytes, ABIType, Format> fromUser(std::string const& _arg, Tristate _prefix, Tristate _typing);

struct ABIMethod
{
	std::string name;
	std::vector<ABIType> ins;
	std::vector<ABIType> outs;
	bool isConstant = false;
	
	// isolation *IS* documentation.
	
	ABIMethod() = default;
	
	ABIMethod(json_spirit::mObject _o);
	
	ABIMethod(std::string const& _name, std::vector<ABIType> const& _args);
	
	std::string sig() const;
	FixedHash<4> id() const;
	std::string solidityDeclaration() const;
	
	bytes encode(std::vector<std::pair<bytes, Format>> const& _params) const;
	std::string decode(bytes const& _data, int _index, EncodingPrefs _ep);
};

class ABI
{
public:
	ABI() = default;
	ABI(std::string const& _json);
	
	ABIMethod method(std::string _nameOrSig, std::vector<ABIType> const& _args) const;
	
	friend std::ostream& operator<<(std::ostream& _out, ABI const& _abi);
	
private:
	std::map<FixedHash<4>, ABIMethod> m_methods;
};

std::ostream& operator<<(std::ostream& _out, ABI const& _abi);

void userOutput(std::ostream& _out, bytes const& _data, Encoding _e);
	
}
}
