// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "Interface.h"

using namespace std;
using namespace dev;
using namespace eth;

void Interface::submitTransaction(Secret const& _secret, u256 const& _value, Address const& _dest, bytes const& _data, u256 const& _gas, u256 const& _gasPrice, u256 const& _nonce)
{
	TransactionSkeleton ts;
	ts.creation = false;
	ts.value = _value;
	ts.to = _dest;
	ts.data = _data;
	ts.gas = _gas;
	ts.gasPrice = _gasPrice;
	ts.nonce = _nonce;
	submitTransaction(ts, _secret);
}

BlockHeader Interface::blockInfo(BlockNumber _block) const
{
	if (_block == PendingBlock)
		return pendingInfo();
	return blockInfo(hashFromNumber(_block));
}

BlockDetails Interface::blockDetails(BlockNumber _block) const
{
	if (_block == PendingBlock)
		return pendingDetails();
	return blockDetails(hashFromNumber(_block));
}
