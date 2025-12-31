// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "FixedHash.h"
#include <boost/algorithm/string.hpp>

namespace dev
{

std::random_device s_fixedHashEngine;

h128 fromUUID(std::string const& _uuid)
{
	try
	{
		return h128(boost::replace_all_copy(_uuid, "-", ""));
	}
	catch (...)
	{
		return h128();
	}
}

std::string toUUID(h128 const& _uuid)
{
	std::string ret = toHex(_uuid.ref());
	for (unsigned i: {20, 16, 12, 8})
		ret.insert(ret.begin() + i, '-');
	return ret;
}

}



