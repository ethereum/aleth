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

#pragma once

#include <string>
#include <vector>
#include <QString>
#include <QList>
#include <libethcore/CommonEth.h>

class QComboBox;

namespace dev { namespace eth { class StateDiff; } }

#define Small "font-size: small; "
#define Mono "font-family: Ubuntu Mono, Monospace, Lucida Console, Courier New; font-weight: bold; "
#define Div(S) "<div style=\"" S "\">"
#define Span(S) "<span style=\"" S "\">"

void initUnits(QComboBox* _b);

std::vector<dev::KeyPair> keysAsVector(QList<dev::KeyPair> const& _keys);

bool sourceIsSolidity(std::string const& _source);
bool sourceIsSerpent(std::string const& _source);

class NatSpecFace
{
public:
	virtual ~NatSpecFace();

	virtual void add(dev::h256 const& _contractHash, std::string const& _doc) = 0;
	virtual std::string retrieve(dev::h256 const& _contractHash) const = 0;
	virtual std::string getUserNotice(std::string const& json, const dev::bytes& _transactionData) = 0;
	virtual std::string getUserNotice(dev::h256 const& _contractHash, dev::bytes const& _transactionDacta) = 0;
};

class Context
{
public:
	virtual ~Context();

	virtual QString pretty(dev::Address _a) const = 0;
	virtual QString prettyU256(dev::u256 _n) const = 0;
	virtual QString render(dev::Address _a) const = 0;
	virtual dev::Address fromString(QString const& _a) const = 0;
	virtual std::string renderDiff(dev::eth::StateDiff const& _d) const = 0;
};

