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
/** @file CodeFragment.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "CodeFragment.h"

#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libevmcore/Instruction.h>
#include "CompilerState.h"
#include "Parser.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace qi = boost::spirit::qi;
namespace px = boost::phoenix;
namespace sp = boost::spirit;

void CodeFragment::finalise(CompilerState const& _cs)
{
	if (_cs.usedAlloc && _cs.vars.size() && !m_finalised)
	{
		m_finalised = true;
		m_asm.injectStart(Instruction::MSTORE8);
		m_asm.injectStart((u256)((_cs.vars.size() + 2) * 32) - 1);
		m_asm.injectStart((u256)1);
	}
}

CodeFragment::CodeFragment(sp::utree const& _t, CompilerState& _s, bool _allowASM)
{
/*	cdebug << "CodeFragment. Locals:";
	for (auto const& i: _s.defs)
		cdebug << i.first << ":" << toHex(i.second.m_code);
	cdebug << "Args:";
	for (auto const& i: _s.args)
		cdebug << i.first << ":" << toHex(i.second.m_code);
	cdebug << "Outers:";
	for (auto const& i: _s.outers)
		cdebug << i.first << ":" << toHex(i.second.m_code);
	debugOutAST(cout, _t);
	cout << endl << flush;
*/
	switch (_t.which())
	{
	case sp::utree_type::list_type:
		constructOperation(_t, _s);
		break;
	case sp::utree_type::string_type:
	{
		auto sr = _t.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::string_type>>();
		string s(sr.begin(), sr.end());
		m_asm.append(s);
		break;
	}
	case sp::utree_type::symbol_type:
	{
		auto sr = _t.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::symbol_type>>();
		string s(sr.begin(), sr.end());
		string us = boost::algorithm::to_upper_copy(s);
		if (_allowASM && c_instructions.count(us))
			m_asm.append(c_instructions.at(us));
		else if (_s.defs.count(s))
			m_asm.append(_s.defs.at(s).m_asm);
		else if (_s.args.count(s))
			m_asm.append(_s.args.at(s).m_asm);
		else if (_s.outers.count(s))
			m_asm.append(_s.outers.at(s).m_asm);
		else if (us.find_first_of("1234567890") != 0 && us.find_first_not_of("QWERTYUIOPASDFGHJKLZXCVBNM1234567890_") == string::npos)
		{
			auto it = _s.vars.find(s);
			if (it == _s.vars.end())
			{
				bool ok;
				tie(it, ok) = _s.vars.insert(make_pair(s, make_pair(_s.stackSize, 32)));
				_s.stackSize += 32;
			}
			m_asm.append((u256)it->second.first);
		}
		else
			error<BareSymbol>();

		break;
	}
	case sp::utree_type::any_type:
	{
		bigint i = *_t.get<bigint*>();
		if (i < 0 || i > bigint(u256(0) - 1))
			error<IntegerOutOfRange>();
		m_asm.append((u256)i);
		break;
	}
	default: break;
	}
}

void CodeFragment::constructOperation(sp::utree const& _t, CompilerState& _s)
{
	if (_t.tag() == 0 && _t.empty())
		error<EmptyList>();
	else if (_t.tag() == 0 && _t.front().which() != sp::utree_type::symbol_type)
		error<DataNotExecutable>();
	else
	{
		string s;
		string us;
		switch (_t.tag())
		{
		case 0:
		{
			auto sr = _t.front().get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::symbol_type>>();
			s = string(sr.begin(), sr.end());
			us = boost::algorithm::to_upper_copy(s);
			break;
		}
		case 1:
			us = "MLOAD";
			break;
		case 2:
			us = "SLOAD";
			break;
		case 3:
			us = "MSTORE";
			break;
		case 4:
			us = "SSTORE";
			break;
		case 5:
			us = "SEQ";
			break;
		case 6:
			us = "CALLDATALOAD";
			break;
		default:;
		}

		auto firstAsString = [&]()
		{
			auto i = *++_t.begin();
			if (i.tag())
				error<InvalidName>();
			if (i.which() == sp::utree_type::string_type)
			{
				auto sr = i.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::string_type>>();
				return string(sr.begin(), sr.end());
			}
			else if (i.which() == sp::utree_type::symbol_type)
			{
				auto sr = i.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::symbol_type>>();
				return _s.getDef(string(sr.begin(), sr.end())).m_asm.backString();
			}
			return string();
		};

		auto varAddress = [&](string const& n)
		{
			auto it = _s.vars.find(n);
			if (it == _s.vars.end())
			{
				bool ok;
				tie(it, ok) = _s.vars.insert(make_pair(n, make_pair(_s.stackSize, 32)));
				_s.stackSize += 32;
			}
			return it->second.first;
		};

		// Operations who args are not standard stack-pushers.
		bool nonStandard = true;
		if (us == "ASM")
		{
			int c = 0;
			for (auto const& i: _t)
				if (c++)
					m_asm.append(CodeFragment(i, _s, true).m_asm);
		}
		else if (us == "INCLUDE")
		{
			if (_t.size() != 2)
				error<IncorrectParameterCount>();
			m_asm.append(CodeFragment::compile(asString(contents(firstAsString())), _s).m_asm);
		}
		else if (us == "SET")
		{
			if (_t.size() != 3)
				error<IncorrectParameterCount>();
			int c = 0;
			for (auto const& i: _t)
				if (c++ == 2)
					m_asm.append(CodeFragment(i, _s, false).m_asm);
			m_asm.append((u256)varAddress(firstAsString()));
			m_asm.append(Instruction::MSTORE);
		}
		else if (us == "GET")
		{
			if (_t.size() != 2)
				error<IncorrectParameterCount>();
			m_asm.append((u256)varAddress(firstAsString()));
			m_asm.append(Instruction::MLOAD);
		}
		else if (us == "REF")
			m_asm.append((u256)varAddress(firstAsString()));
		else if (us == "DEF")
		{
			string n;
			unsigned ii = 0;
			if (_t.size() != 3 && _t.size() != 4)
				error<IncorrectParameterCount>();
			vector<string> args;
			for (auto const& i: _t)
			{
				if (ii == 1)
				{
					if (i.tag())
						error<InvalidName>();
					if (i.which() == sp::utree_type::string_type)
					{
						auto sr = i.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::string_type>>();
						n = string(sr.begin(), sr.end());
					}
					else if (i.which() == sp::utree_type::symbol_type)
					{
						auto sr = i.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::symbol_type>>();
						n = _s.getDef(string(sr.begin(), sr.end())).m_asm.backString();
					}
				}
				else if (ii == 2)
					if (_t.size() == 3)
						_s.defs[n] = CodeFragment(i, _s);
					else
						for (auto const& j: i)
						{
							if (j.tag() || j.which() != sp::utree_type::symbol_type)
								error<InvalidMacroArgs>();
							auto sr = j.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::symbol_type>>();
							args.push_back(string(sr.begin(), sr.end()));
						}
				else if (ii == 3)
				{
					auto k = make_pair(n, args.size());
					_s.macros[k].code = i;
					_s.macros[k].env = _s.outers;
					_s.macros[k].args = args;
					for (auto const& i: _s.args)
						_s.macros[k].env[i.first] = i.second;
					for (auto const& i: _s.defs)
						_s.macros[k].env[i.first] = i.second;
				}
				++ii;
			}

		}
		else if (us == "LIT")
		{
			if (_t.size() < 3)
				error<IncorrectParameterCount>();
			unsigned ii = 0;
			CodeFragment pos;
			bytes data;
			for (auto const& i: _t)
			{
				if (ii == 1)
				{
					pos = CodeFragment(i, _s);
					if (pos.m_asm.deposit() != 1)
						error<InvalidDeposit>();
				}
				else if (ii == 2 && !i.tag() && i.which() == sp::utree_type::string_type)
				{
					auto sr = i.get<sp::basic_string<boost::iterator_range<char const*>, sp::utree_type::string_type>>();
					data = bytes((byte const*)sr.begin(), (byte const*)sr.end());
				}
				else if (ii >= 2 && !i.tag() && i.which() == sp::utree_type::any_type)
				{
					bigint bi = *i.get<bigint*>();
					if (bi < 0)
						error<IntegerOutOfRange>();
					else if (bi > bigint(u256(0) - 1))
					{
						if (ii == 2 && _t.size() == 3)
						{
							// One big int - allow it as hex.
							data.resize(bytesRequired(bi));
							toBigEndian(bi, data);
						}
						else
							error<IntegerOutOfRange>();
					}
					else
					{
						data.resize(data.size() + 32);
						*(h256*)(&data.back() - 31) = (u256)bi;
					}
				}
				else if (ii)
					error<InvalidLiteral>();
				++ii;
			}
			m_asm.append((u256)data.size());
			m_asm.append(Instruction::DUP1);
			m_asm.append(data);
			m_asm.append(pos.m_asm, 1);
			m_asm.append(Instruction::CODECOPY);
		}
		else
			nonStandard = false;

		if (nonStandard)
			return;

		std::map<std::string, Instruction> const c_arith = { { "+", Instruction::ADD }, { "-", Instruction::SUB }, { "*", Instruction::MUL }, { "/", Instruction::DIV }, { "%", Instruction::MOD }, { "&", Instruction::AND }, { "|", Instruction::OR }, { "^", Instruction::XOR } };
		std::map<std::string, pair<Instruction, bool>> const c_binary = { { "<", { Instruction::LT, false } }, { "<=", { Instruction::GT, true } }, { ">", { Instruction::GT, false } }, { ">=", { Instruction::LT, true } }, { "S<", { Instruction::SLT, false } }, { "S<=", { Instruction::SGT, true } }, { "S>", { Instruction::SGT, false } }, { "S>=", { Instruction::SLT, true } }, { "=", { Instruction::EQ, false } }, { "!=", { Instruction::EQ, true } } };
		std::map<std::string, Instruction> const c_unary = { { "!", Instruction::ISZERO } };

		vector<CodeFragment> code;
		CompilerState ns = _s;
		ns.vars.clear();
		ns.usedAlloc = false;
		int c = _t.tag() ? 1 : 0;
		for (auto const& i: _t)
			if (c++)
			{
				if (us == "LLL" && c == 1)
					code.push_back(CodeFragment(i, ns));
				else
					code.push_back(CodeFragment(i, _s));
			}
		auto requireSize = [&](unsigned s) { if (code.size() != s) error<IncorrectParameterCount>(); };
		auto requireMinSize = [&](unsigned s) { if (code.size() < s) error<IncorrectParameterCount>(); };
		auto requireMaxSize = [&](unsigned s) { if (code.size() > s) error<IncorrectParameterCount>(); };
		auto requireDeposit = [&](unsigned i, int s) { if (code[i].m_asm.deposit() != s) error<InvalidDeposit>(); };

		if (_s.macros.count(make_pair(s, code.size())))
		{
			Macro const& m = _s.macros.at(make_pair(s, code.size()));
			CompilerState cs = _s;
			for (auto const& i: m.env)
				cs.outers[i.first] = i.second;
			for (auto const& i: cs.defs)
				cs.outers[i.first] = i.second;
			cs.defs.clear();
			for (unsigned i = 0; i < m.args.size(); ++i)
			{
				//requireDeposit(i, 1);
				cs.args[m.args[i]] = code[i];
			}
			m_asm.append(CodeFragment(m.code, cs).m_asm);
			for (auto const& i: cs.defs)
				_s.defs[i.first] = i.second;
			for (auto const& i: cs.macros)
				_s.macros.insert(i);
		}
		else if (c_instructions.count(us))
		{
			auto it = c_instructions.find(us);
			int ea = instructionInfo(it->second).args;
			if (ea >= 0)
				requireSize(ea);
			else
				requireMinSize(-ea);

			for (unsigned i = code.size(); i; --i)
				m_asm.append(code[i - 1].m_asm, 1);
			m_asm.append(it->second);
		}
		else if (c_arith.count(us))
		{
			auto it = c_arith.find(us);
			requireMinSize(1);
			for (unsigned i = code.size(); i; --i)
			{
				requireDeposit(i - 1, 1);
				m_asm.append(code[i - 1].m_asm, 1);
			}
			for (unsigned i = 1; i < code.size(); ++i)
				m_asm.append(it->second);
		}
		else if (c_binary.count(us))
		{
			auto it = c_binary.find(us);
			requireSize(2);
			requireDeposit(0, 1);
			requireDeposit(1, 1);
			m_asm.append(code[1].m_asm, 1);
			m_asm.append(code[0].m_asm, 1);
			m_asm.append(it->second.first);
			if (it->second.second)
				m_asm.append(Instruction::ISZERO);
		}
		else if (c_unary.count(us))
		{
			auto it = c_unary.find(us);
			requireSize(1);
			requireDeposit(0, 1);
			m_asm.append(code[0].m_asm, 1);
			m_asm.append(it->second);
		}
		else if (us == "IF")
		{
			requireSize(3);
			requireDeposit(0, 1);
			int minDep = min(code[1].m_asm.deposit(), code[2].m_asm.deposit());

			m_asm.append(code[0].m_asm);
			auto pos = m_asm.appendJumpI();
			m_asm.onePath();
			m_asm.append(code[2].m_asm, minDep);
			auto end = m_asm.appendJump();
			m_asm.otherPath();
			m_asm << pos.tag();
			m_asm.append(code[1].m_asm, minDep);
			m_asm << end.tag();
			m_asm.donePaths();
		}
		else if (us == "WHEN" || us == "UNLESS")
		{
			requireSize(2);
			requireDeposit(0, 1);

			m_asm.append(code[0].m_asm);
			if (us == "WHEN")
				m_asm.append(Instruction::ISZERO);
			auto end = m_asm.appendJumpI();
			m_asm.onePath();
			m_asm.otherPath();
			m_asm.append(code[1].m_asm, 0);
			m_asm << end.tag();
			m_asm.donePaths();
		}
		else if (us == "WHILE")
		{
			requireSize(2);
			requireDeposit(0, 1);

			auto begin = m_asm.append();
			m_asm.append(code[0].m_asm);
			m_asm.append(Instruction::ISZERO);
			auto end = m_asm.appendJumpI();
			m_asm.append(code[1].m_asm, 0);
			m_asm.appendJump(begin);
			m_asm << end.tag();
		}
		else if (us == "FOR")
		{
			requireSize(4);
			requireDeposit(1, 1);

			m_asm.append(code[0].m_asm, 0);
			auto begin = m_asm.append();
			m_asm.append(code[1].m_asm);
			m_asm.append(Instruction::ISZERO);
			auto end = m_asm.appendJumpI();
			m_asm.append(code[3].m_asm, 0);
			m_asm.append(code[2].m_asm, 0);
			m_asm.appendJump(begin);
			m_asm << end.tag();
		}
		else if (us == "ALLOC")
		{
			requireSize(1);
			requireDeposit(0, 1);

			m_asm.append(Instruction::MSIZE);
			m_asm.append(u256(0));
			m_asm.append(u256(1));
			m_asm.append(code[0].m_asm, 1);
			m_asm.append(Instruction::MSIZE);
			m_asm.append(Instruction::ADD);
			m_asm.append(Instruction::SUB);
			m_asm.append(Instruction::MSTORE8);

			_s.usedAlloc = true;
		}
		else if (us == "LLL")
		{
			requireMinSize(2);
			requireMaxSize(3);
			requireDeposit(1, 1);

			auto subPush = m_asm.appendSubSize(code[0].assembly(ns));
			m_asm.append(Instruction::DUP1);
			if (code.size() == 3)
			{
				requireDeposit(2, 1);
				m_asm.append(code[2].m_asm, 1);
				m_asm.append(Instruction::LT);
				m_asm.append(Instruction::ISZERO);
				m_asm.append(Instruction::MUL);
				m_asm.append(Instruction::DUP1);
			}
			m_asm.append(subPush);
			m_asm.append(code[1].m_asm, 1);
			m_asm.append(Instruction::CODECOPY);
		}
		else if (us == "&&" || us == "||")
		{
			requireMinSize(1);
			for (unsigned i = 0; i < code.size(); ++i)
				requireDeposit(i, 1);

			auto end = m_asm.newTag();
			if (code.size() > 1)
			{
				m_asm.append((u256)(us == "||" ? 1 : 0));
				for (unsigned i = 1; i < code.size(); ++i)
				{
					// Check if true - predicate
					m_asm.append(code[i - 1].m_asm, 1);
					if (us == "&&")
						m_asm.append(Instruction::ISZERO);
					m_asm.appendJumpI(end);
				}
				m_asm.append(Instruction::POP);
			}

			// Check if true - predicate
			m_asm.append(code.back().m_asm, 1);

			// At end now.
			m_asm.append(end);
		}
		else if (us == "~")
		{
			requireSize(1);
			requireDeposit(0, 1);

			m_asm.append(code[0].m_asm, 1);
			m_asm.append((u256)1);
			m_asm.append((u256)0);
			m_asm.append(Instruction::SUB);
			m_asm.append(Instruction::SUB);
		}
		else if (us == "SEQ")
		{
			unsigned ii = 0;
			for (auto const& i: code)
				if (++ii < code.size())
					m_asm.append(i.m_asm, 0);
				else
					m_asm.append(i.m_asm);
		}
		else if (us == "RAW")
		{
			for (auto const& i: code)
				m_asm.append(i.m_asm);
			m_asm.popTo(1);
		}
		else if (us.find_first_of("1234567890") != 0 && us.find_first_not_of("QWERTYUIOPASDFGHJKLZXCVBNM1234567890_") == string::npos)
			m_asm.append((u256)varAddress(s));
		else
			error<InvalidOperation>();
	}
}

CodeFragment CodeFragment::compile(string const& _src, CompilerState& _s)
{
	CodeFragment ret;
	sp::utree o;
	parseTreeLLL(_src, o);
	if (!o.empty())
		ret = CodeFragment(o, _s);
	_s.treesToKill.push_back(o);
	return ret;
}
