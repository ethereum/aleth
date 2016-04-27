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
/** @file Debugger.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include <cstdlib>
#include "Common.h"
#include <QComboBox>
#include <QSpinBox>
#include <libethcore/Common.h>
#include <libdevcore/Log.h>
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

NatSpecFace::~NatSpecFace()
{
}

string dev::niceVersion(string const& _v)
{
	if (_v.substr(1, 4) == ".9.9" && _v.size() >= 6)
		return toString(stoi(_v.substr(0, 1)) + 1) + ".0rc" + _v.substr(5);
	return _v;
}

string dev::aleth::fromRaw(h256 const& _n, unsigned* _inc)
{
	if (_n)
	{
		string s((char const*)_n.data(), 32);
		auto l = s.find_first_of('\0');
		if (!l)
			return string();
		if (l != string::npos)
		{
			auto p = s.find_first_not_of('\0', l);
			if (!(p == string::npos || (_inc && p == 31)))
				return string();
			if (_inc)
				*_inc = (byte)s[31];
			s.resize(l);
		}
		for (auto i: s)
			if (i < 32)
				return string();
		return s;
	}
	return string();
}

void dev::aleth::setValueUnits(QComboBox* _units, QSpinBox* _value, u256 _v)
{
	initUnits(_units);
	if (_v > 0)
	{
		_units->setCurrentIndex(0);
		while (_v > 50000 && _units->currentIndex() < (int)(units().size() - 2))
		{
			_v /= 1000;
			_units->setCurrentIndex(_units->currentIndex() + 1);
		}
	}
	else
		_units->setCurrentIndex(6);
	_value->setValue((unsigned)_v);
}

u256 dev::aleth::fromValueUnits(QComboBox* _units, QSpinBox* _value)
{
	return _value->value() * units()[units().size() - 1 - _units->currentIndex()].first;
}

void dev::aleth::initUnits(QComboBox* _b)
{
	_b->clear();
	for (auto n = (unsigned)units().size(); n-- != 0; )
		_b->addItem(QString::fromStdString(units()[n].second), n);
}

vector<KeyPair> dev::aleth::keysAsVector(QList<KeyPair> const& keys)
{
	auto list = keys.toStdList();
	return {begin(list), end(list)};
}

bool dev::aleth::sourceIsSolidity(string const& _source)
{
	// TODO: Improve this heuristic
	return (_source.substr(0, 8) == "contract" || _source.substr(0, 5) == "//sol");
}
