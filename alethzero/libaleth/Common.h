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
#include <QString>
#include <QList>
#include <libethcore/Common.h>

class QComboBox;
class QSpinBox;

namespace dev
{

// TODO: move to libdevcore when you fancy a big recompile.
std::string niceVersion(std::string const& _v);

namespace aleth
{

#define ETH_HTML_SMALL "font-size: small; "
#define ETH_HTML_MONO "font-family: Ubuntu Mono, Monospace, Lucida Console, Courier New; font-weight: bold; "
#define ETH_HTML_DIV(S) "<div style=\"" S "\">"
#define ETH_HTML_SPAN(S) "<span style=\"" S "\">"

/// Simple HTML escaping utility function for STL strings.
inline std::string htmlEscaped(std::string const& _s) { return QString::fromStdString(_s).toHtmlEscaped().toStdString(); }
/// Get a string from a raw h256 value.
std::string fromRaw(h256 const& _n, unsigned* _inc);

void initUnits(QComboBox* _b);
void setValueUnits(QComboBox* _units, QSpinBox* _value, dev::u256 _v);
dev::u256 fromValueUnits(QComboBox* _units, QSpinBox* _value);

std::vector<dev::KeyPair> keysAsVector(QList<dev::KeyPair> const& _keys);

bool sourceIsSolidity(std::string const& _source);

class NatSpecFace
{
public:
	virtual ~NatSpecFace();

	virtual void add(dev::h256 const& _contractHash, std::string const& _doc) = 0;
	virtual std::string retrieve(dev::h256 const& _contractHash) const = 0;
	virtual std::string userNotice(std::string const& json, const dev::bytes& _transactionData) = 0;
	virtual std::string userNotice(dev::h256 const& _contractHash, dev::bytes const& _transactionDacta) = 0;
};

}
}
