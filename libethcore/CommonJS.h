// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <string>
#include <libdevcore/CommonJS.h>
#include <libdevcrypto/Common.h>
#include "Common.h"

// devcrypto

namespace dev
{

/// Leniently convert string to Public (h512). Accepts integers, "0x" prefixing, non-exact length.
inline Public jsToPublic(std::string const& _s) { return jsToFixed<sizeof(dev::Public)>(_s); }

/// Leniently convert string to Secret (h256). Accepts integers, "0x" prefixing, non-exact length.
inline Secret jsToSecret(std::string const& _s) { h256 d = jsToFixed<sizeof(dev::Secret)>(_s); Secret ret(d); d.ref().cleanse(); return ret; }

/// Leniently convert string to Address (h160). Accepts integers, "0x" prefixing, non-exact length.
inline Address jsToAddress(std::string const& _s) { return eth::toAddress(_s); }

/// Convert u256 into user-readable string. Returns int/hex value of 64 bits int, hex of 160 bits FixedHash. As a fallback try to handle input as h256.
std::string prettyU256(u256 _n, bool _abridged = true);

}


// ethcore
namespace dev
{
namespace eth
{

/// Convert to a block number, a bit like jsToInt, except that it correctly recognises "pending" and "latest".
BlockNumber jsToBlockNumber(std::string const& _js);

}
}
