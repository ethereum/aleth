// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "FixedClient.h"

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

Block FixedClient::block(h256 const& _h) const
{
	ReadGuard l(x_stateDB);
	Block ret(bc(), m_block.db());
	ret.populateFromChain(bc(), _h);
	return ret;
}
