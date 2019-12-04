// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "BlockDetails.h"

#include <libdevcore/Common.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

BlockDetails::BlockDetails(RLP const& _r)
{
    number = _r[0].toInt<unsigned>();
    totalDifficulty = _r[1].toInt<u256>();
    parentHash = _r[2].toHash<h256>();
    childHashes = _r[3].toVector<h256>();
    size = _r.size();
    blockSizeBytes = _r[4].toInt<size_t>();
}

bytes BlockDetails::rlp() const
{
    auto const detailsRlp =
        rlpList(number, totalDifficulty, parentHash, childHashes, blockSizeBytes);
    size = detailsRlp.size();
    return detailsRlp;
}
