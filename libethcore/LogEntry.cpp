/*
	This file is part of aleth.

	aleth is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	aleth is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LogEntry.h"

#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>

namespace dev
{
namespace eth
{

LogEntry::LogEntry(RLP const& _r)
{
	assert(_r.itemCount() == 3);
	address = (Address)_r[0];
	topics = _r[1].toVector<h256>();
	data = _r[2].toBytes();
}

void LogEntry::streamRLP(RLPStream& _s) const
{
	_s.appendList(3) << address << topics << data;
}

LogBloom LogEntry::bloom() const
{
	LogBloom ret;
	ret.shiftBloom<3>(sha3(address.ref()));
	for (auto t: topics)
		ret.shiftBloom<3>(sha3(t.ref()));
	return ret;
}

}
}
