// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
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
