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

#include "Context.h"
#include <QComboBox>
#include <QSpinBox>
#include <libethcore/Common.h>
#include <libethcore/ICAP.h>
#include <libethereum/AccountDiff.h>
#include "Common.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

Context::~Context()
{
}

std::string Context::toHTML(dev::u256 const& _n) const
{
	unsigned inc = 0;
	string raw;
	ostringstream s;
	if (_n > szabo && _n < 1000000 * ether)
		s << "<span style=\"color: #215\">" << formatBalance(_n) << "</span> <span style=\"color: #448\">(0x" << hex << (uint64_t)_n << ")</span>";
	else if (!(_n >> 64))
		s << "<span style=\"color: #008\">" << (uint64_t)_n << "</span> <span style=\"color: #448\">(0x" << hex << (uint64_t)_n << ")</span>";
	else if (!~(_n >> 64))
		s << "<span style=\"color: #008\">" << (int64_t)_n << "</span> <span style=\"color: #448\">(0x" << hex << (int64_t)_n << ")</span>";
	else if ((_n >> 160) == 0)
	{
		Address a = right160(_n);
		string n = toName(a);
		if (n.empty())
			s << "<span style=\"color: #844\">0x</span><span style=\"color: #800\">" << a << "</span>";
		else
			s << "<span style=\"font-weight: bold; color: #800\">" << htmlEscaped(n) << "</span> (<span style=\"color: #844\">0x</span><span style=\"color: #800\">" << a.abridged() << "</span>)";
	}
	else if ((raw = fromRaw((h256)_n, &inc)).size())
		return "<span style=\"color: #484\">\"</span><span style=\"color: #080\">" + htmlEscaped(raw) + "</span><span style=\"color: #484\">\"" + (inc ? " + " + toString(inc) : "") + "</span>";
	else
		s << "<span style=\"color: #466\">0x</span><span style=\"color: #044\">" << (h256)_n << "</span>";
	return s.str();
}

std::string Context::toReadable(Address const& _a) const
{
	string ret = ICAP(_a).encoded();
	string n = toName(_a);
	if (!n.empty())
		ret = n + " (" + ret + ")";
	return ret;
}
