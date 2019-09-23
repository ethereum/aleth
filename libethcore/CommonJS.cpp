// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "CommonJS.h"

namespace dev
{

std::string prettyU256(u256 _n, bool _abridged)
{
	std::string raw;
	std::ostringstream s;
	if (!(_n >> 64))
		s << " " << (uint64_t)_n << " (0x" << std::hex << (uint64_t)_n << ")";
	else if (!~(_n >> 64))
		s << " " << (int64_t)_n << " (0x" << std::hex << (int64_t)_n << ")";
	else if ((_n >> 160) == 0)
	{
		Address a = right160(_n);

		std::string n;
		if (_abridged)
			n =  a.abridged();
		else
			n = toHex(a.ref());

		if (n.empty())
			s << "0";
		else
			s << _n << "(0x" << n << ")";
	}
	else if (!(raw = fromRaw((h256)_n)).empty())
		return "\"" + raw + "\"";
	else
		s << "" << (h256)_n;
	return s.str();
}

namespace eth
{

BlockNumber jsToBlockNumber(std::string const& _js)
{
	if (_js == "latest")
		return LatestBlock;
	else if (_js == "earliest")
		return 0;
	else if (_js == "pending")
		return PendingBlock;
	else
		return (unsigned)jsToInt(_js);
}

}

}

