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
/** Abi.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include <fstream>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "../test/JsonSpiritHeaders.h"
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/SHA3.h>
#include <libethereum/Client.h>
#include "Abi.h"

using namespace std;
using namespace dev;
using namespace dev::abi;
namespace js = json_spirit;

struct InvalidUserString: public Exception {};
struct InvalidFormat: public Exception {};

static const map<Base, string> s_bases =
{
	{ Base::Bytes, "bytes" },
	{ Base::Address, "address" },
	{ Base::Int, "int" },
	{ Base::Uint, "uint" },
	{ Base::Fixed, "fixed" }
};


ABIType::ABIType(std::string const& _type, std::string const& _name):
	name(_name)
{
	string rest;
	for (auto const& i: s_bases)
		if (boost::algorithm::starts_with(_type, i.second))
		{
			base = i.first;
			rest = _type.substr(i.second.size());
		}
	if (base == Base::Unknown)
		throw InvalidFormat();
	boost::regex r("(\\d*)(x(\\d+))?((\\[\\d*\\])*)");
	boost::smatch res;
	boost::regex_match(rest, res, r);
	size = res[1].length() > 0 ? stoi(res[1]) : 0;
	ssize = res[3].length() > 0 ? stoi(res[3]) : 0;
	boost::regex r2("\\[(\\d*)\\](.*)");
	for (rest = res[4]; boost::regex_match(rest, res, r2); rest = res[2])
		dims.push_back(!res[1].length() ? -1 : stoi(res[1]));
}

ABIType::ABIType(std::string const& _s)
{
	if (_s.size() < 1)
		return;
	switch (_s[0])
	{
		case 'b': base = Base::Bytes; break;
		case 'a': base = Base::Address; break;
		case 'i': base = Base::Int; break;
		case 'u': base = Base::Uint; break;
		case 'f': base = Base::Fixed; break;
		default: throw InvalidFormat();
	}
	if (_s.size() < 2)
	{
		if (base == Base::Fixed)
			size = ssize = 16;
		else if (base == Base::Address || base == Base::Bytes)
			size = 0;
		else
			size = 32;
		return;
	}
	strings d;
	boost::algorithm::split(d, _s, boost::is_any_of("*"));
	string s = d[0];
	if (s.find_first_of('x') == string::npos)
		size = stoi(s.substr(1));
	else
	{
		size = stoi(s.substr(1, s.find_first_of('x') - 1));
		ssize = stoi(s.substr(s.find_first_of('x') + 1));
	}
	for (unsigned i = 1; i < d.size(); ++i)
		if (d[i].empty())
			dims.push_back(-1);
		else
			dims.push_back(stoi(d[i]));
}

string ABIType::canon() const
{
	string ret;
	switch (base)
	{
		case Base::Bytes: ret = "bytes" + (size > 0 ? toString(size) : ""); break;
		case Base::Address: ret = "address"; break;
		case Base::Int: ret = "int" + toString(size); break;
		case Base::Uint: ret = "uint" + toString(size); break;
		case Base::Fixed: ret = "fixed" + toString(size) + "x" + toString(ssize); break;
		default: throw InvalidFormat();
	}
	for (int i: dims)
		ret += "[" + ((i > -1) ? toString(i) : "") + "]";
	return ret;
}

bool ABIType::isBytes() const { return base == Base::Bytes && !size; }
	
string ABIType::render(bytes const& _data, EncodingPrefs _e) const
{
	if (base == Base::Uint || base == Base::Int)
	{
		if (_e.e == Encoding::Hex)
			return (_e.prefix ? "0x" : "") + toHex(toCompactBigEndian(fromBigEndian<bigint>(bytesConstRef(&_data).cropped(32 - size / 8))));
		else
		{
			bigint i = fromBigEndian<bigint>(bytesConstRef(&_data).cropped(32 - size / 8));
			if (base == Base::Int && i > (bigint(1) << (size - 1)))
				i -= (bigint(1) << size);
			return toString(i);
		}
	}
	else if (base == Base::Address)
	{
		Address a = Address(h256(_data), Address::AlignRight);
		return _e.e == Encoding::Binary ? asString(a.asBytes()) : ((_e.prefix ? "0x" : "") + toString(a));
	}
	else if (isBytes())
	{
		return _e.e == Encoding::Binary ? asString(_data) : ((_e.prefix ? "0x" : "") + toHex(_data));
	}
	else if (base == Base::Bytes)
	{
		bytesConstRef b(&_data);
		b = b.cropped(0, size);
		return _e.e == Encoding::Binary ? asString(b) : ((_e.prefix ? "0x" : "") + toHex(b));
	}
	else
		throw InvalidFormat();
}

bytes ABIType::unrender(bytes const& _data, Format _f) const
{
	if (isBytes())
	{
		auto ret = _data;
		while (ret.size() % 32 != 0)
			ret.push_back(0);
		return ret;
	}
	else
		return aligned(_data, _f, 32);
}

void ABIType::noteHexInput(unsigned _nibbles) { if (base == Base::Unknown) { if (_nibbles == 40) base = Base::Address; else { base = Base::Bytes; size = _nibbles / 2; } } }
void ABIType::noteBinaryInput() { if (base == Base::Unknown) { base = Base::Bytes; size = 32; } }
void ABIType::noteDecimalInput() { if (base == Base::Unknown) { base = Base::Uint; size = 32; } }
	
bytes ABIType::aligned(bytes const& _b, Format _f, unsigned _length) const
{
	bytes ret = _b;
	while (ret.size() < _length)
		if (base == Base::Bytes || (base == Base::Unknown && _f == Format::Binary))
			ret.push_back(0);
		else
			ret.insert(ret.begin(), 0);
	while (ret.size() > _length)
		if (base == Base::Bytes || (base == Base::Unknown && _f == Format::Binary))
			ret.pop_back();
		else
			ret.erase(ret.begin());
	return ret;
}

tuple<bytes, ABIType, Format> dev::abi::fromUser(std::string const& _arg, Tristate _prefix, Tristate _typing)
{
	ABIType type;
	string val;
	if (_typing == Tristate::True || (_typing == Tristate::Mu && _arg.find(':') != string::npos))
	{
		if (_arg.find(':') == string::npos)
			throw InvalidUserString();
		type = ABIType(_arg.substr(0, _arg.find(':')));
		val = _arg.substr(_arg.find(':') + 1);
	}
	else
		val = _arg;
	
	if (_prefix != Tristate::False)
	{
		if (val.substr(0, 2) == "0x")
		{
			type.noteHexInput(val.size() - 2);
			return make_tuple(fromHex(val), type, Format::Hex);
		}
		if (val.substr(0, 1) == "+")
		{
			type.noteDecimalInput();
			return make_tuple(toCompactBigEndian(bigint(val.substr(1))), type, Format::Decimal);
		}
		if (val.substr(0, 1) == "'")
		{
			type.noteBinaryInput();
			return make_tuple(asBytes(val.substr(1)), type, Format::Binary);
		}
		if (val == "[")
			return make_tuple(bytes(), type, Format::Open);
		if (val == "]")
			return make_tuple(bytes(), type, Format::Close);
	}
	if (_prefix != Tristate::True)
	{
		if (val.find_first_not_of("0123456789") == string::npos)
		{
			type.noteDecimalInput();
			return make_tuple(toCompactBigEndian(bigint(val)), type, Format::Decimal);
		}
		if (val.find_first_not_of("0123456789abcdefABCDEF") == string::npos)
		{
			type.noteHexInput(val.size());
			return make_tuple(fromHex(val), type, Format::Hex);
		}
		if (val == "[")
			return make_tuple(bytes(), type, Format::Open);
		if (val == "]")
			return make_tuple(bytes(), type, Format::Close);
		type.noteBinaryInput();
		return make_tuple(asBytes(val), type, Format::Binary);
	}
	throw InvalidUserString();
}

	
ABIMethod::ABIMethod(js::mObject _o)
{
	name = _o["name"].get_str();
	isConstant = _o["constant"].get_bool();
	if (_o.count("inputs"))
		for (auto const& i: _o["inputs"].get_array())
		{
			js::mObject a = i.get_obj();
			ins.push_back(ABIType(a["type"].get_str(), a["name"].get_str()));
		}
	if (_o.count("outputs"))
		for (auto const& i: _o["outputs"].get_array())
		{
			js::mObject a = i.get_obj();
			outs.push_back(ABIType(a["type"].get_str(), a["name"].get_str()));
		}
}

ABIMethod::ABIMethod(string const& _name, vector<ABIType> const& _args)
{
	name = _name;
	ins = _args;
}

string ABIMethod::sig() const
{
	string methodArgs;
	for (auto const& arg: ins)
		methodArgs += (methodArgs.empty() ? "" : ",") + arg.canon();
	return name + "(" + methodArgs + ")";
}
FixedHash<4> ABIMethod::id() const { return FixedHash<4>(sha3(sig())); }
	
std::string ABIMethod::solidityDeclaration() const
{
	ostringstream ss;
	ss << "function " << name << "(";
	int f = 0;
	for (ABIType const& i: ins)
		ss << (f++ ? ", " : "") << i.canon() << " " << i.name;
	ss << ") ";
	if (isConstant)
		ss << "constant ";
	if (!outs.empty())
	{
		ss << "returns(";
		f = 0;
		for (ABIType const& i: outs)
			ss << (f++ ? ", " : "") << i.canon() << " " << i.name;
		ss << ")";
	}
	return ss.str();
}

bytes ABIMethod::encode(vector<pair<bytes, Format>> const& _params) const
{
	bytes ret = name.empty() ? bytes() : id().asBytes();
	bytes suffix;
	
	// int int[] int
	// example: 42 [ 1 2 3 ] 69
	// int[2][][3]
	// example: [ [ [ 1 2 3 ] [ 4 5 6 ] ] [ ] ]
	
	unsigned pi = 0;
	
	for (ABIType const& a: ins)
	{
		if (pi >= _params.size())
			throw ExpectedAdditionalParameter();
		auto put = [&]() {
			if (a.isBytes())
				ret += h256(u256(_params[pi].first.size())).asBytes();
			suffix += a.unrender(_params[pi].first, _params[pi].second);
			pi++;
			if (pi >= _params.size())
				throw ExpectedAdditionalParameter();
		};
		function<void(vector<int>, unsigned)> putDim = [&](vector<int> addr, unsigned q) {
			if (addr.size() == a.dims.size())
				put();
			else
			{
				if (_params[pi].second != Format::Open)
					throw ExpectedOpen();
				++pi;
				int l = a.dims[a.dims.size() - 1 - addr.size()];
				if (l == -1)
				{
					// read ahead in params and discover the arity.
					unsigned depth = 0;
					l = 0;
					for (unsigned pi2 = pi; depth || _params[pi2].second != Format::Close;)
					{
						if (_params[pi2].second == Format::Open)
							++depth;
						if (_params[pi2].second == Format::Close)
							--depth;
						if (!depth)
							++l;
						if (++pi2 == _params.size())
							throw ExpectedClose();
					}
					ret += h256(u256(l)).asBytes();
				}
				q *= l;
				for (addr.push_back(0); addr.back() < l; ++addr.back())
					putDim(addr, q);
				if (_params[pi].second != Format::Close)
					throw ExpectedClose();
				++pi;
			}
		};
		putDim(vector<int>(), 1);
	}
	return ret + suffix;
}
string ABIMethod::decode(bytes const& _data, int _index, EncodingPrefs _ep)
{
	stringstream out;
	unsigned di = 0;
	vector<unsigned> catDims;
	for (ABIType const& a: outs)
	{
		auto put = [&]() {
			if (a.isBytes())
			{
				catDims.push_back(fromBigEndian<unsigned>(bytesConstRef(&_data).cropped(di, 32)));
				di += 32;
			}
		};
		function<void(vector<int>, unsigned)> putDim = [&](vector<int> addr, unsigned q) {
			if (addr.size() == a.dims.size())
				put();
			else
			{
				int l = a.dims[a.dims.size() - 1 - addr.size()];
				if (l == -1)
				{
					l = fromBigEndian<unsigned>(bytesConstRef(&_data).cropped(di, 32));
					catDims.push_back(l);
					di += 32;
				}
				q *= l;
				for (addr.push_back(0); addr.back() < l; ++addr.back())
					putDim(addr, q);
			}
		};
		putDim(vector<int>(), 1);
	}
	unsigned d = 0;
	for (ABIType const& a: outs)
	{
		if (_index == -1 && out.tellp() > 0)
			out << ", ";
		auto put = [&]() {
			if (a.isBytes())
			{
				out << a.render(bytesConstRef(&_data).cropped(di, catDims[d]).toBytes(), _ep);
				di += ((catDims[d] + 31) / 32) * 32;
				d++;
			}
			else
			{
				out << a.render(bytesConstRef(&_data).cropped(di, 32).toBytes(), _ep);
				di += 32;
			}
		};
		function<void(vector<int>)> putDim = [&](vector<int> addr) {
			if (addr.size() == a.dims.size())
				put();
			else
			{
				out << "[";
				addr.push_back(0);
				int l = a.dims[a.dims.size() - 1 - (addr.size() - 1)];
				if (l == -1)
					l = catDims[d++];
				for (addr.back() = 0; addr.back() < l; ++addr.back())
				{
					if (addr.back())
						out << ", ";
					putDim(addr);
				}
				out << "]";
			}
		};
		putDim(vector<int>());
	}
	return out.str();
}

string canonSig(string const& _name, vector<ABIType> const& _args)
{
	try {
		string methodArgs;
		for (auto const& arg: _args)
			methodArgs += (methodArgs.empty() ? "" : ",") + arg.canon();
		return _name + "(" + methodArgs + ")";
	}
	catch (...) {
		return string();
	}
}

ostream& dev::abi::operator<<(ostream& _out, ABI const& _abi)
{
	_out << "contract {" << endl;
	for (auto const& i: _abi.m_methods)
		_out << "  " << i.second.solidityDeclaration() << "; // " << i.first.abridged() << endl;
	_out << "}" << endl;
	return _out;
}

ABI::ABI(std::string const& _json)
{
	js::mValue v;
	js::read_string(_json, v);
	for (auto const& i: v.get_array())
	{
		js::mObject o = i.get_obj();
		if (o["type"].get_str() != "function")
			continue;
		ABIMethod m(o);
		m_methods[m.id()] = m;
	}
}

ABIMethod ABI::method(string _nameOrSig, vector<ABIType> const& _args) const
{
	auto id = FixedHash<4>(sha3(_nameOrSig));
	if (!m_methods.count(id))
		id = FixedHash<4>(sha3(canonSig(_nameOrSig, _args)));
	if (!m_methods.count(id))
		for (auto const& m: m_methods)
			if (m.second.name == _nameOrSig)
			{
				if (m_methods.count(id))
					throw OverloadedMethod();
				id = m.first;
			}
	if (m_methods.count(id))
		return m_methods.at(id);
	throw UnknownMethod();
}

void dev::abi::userOutput(ostream& _out, bytes const& _data, Encoding _e)
{
	switch (_e)
	{
		case Encoding::Binary:
			_out.write((char const*)_data.data(), _data.size());
			break;
		default:
			_out << toHex(_data) << endl;
	}
}

template <unsigned n, class T> vector<typename std::remove_reference<decltype(get<n>(T()))>::type> retrieve(vector<T> const& _t)
{
	vector<typename std::remove_reference<decltype(get<n>(T()))>::type> ret;
	for (T const& i: _t)
		ret.push_back(get<n>(i));
	return ret;
}
