// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// This file defines Address alias for FixedHash of 160 bits and some
/// special Address constants.

#pragma once

#include "FixedHash.h"

namespace dev
{

/// An Ethereum address: 20 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Address = h160;

/// A vector of Ethereum addresses.
using Addresses = h160s;

/// A hash set of Ethereum addresses.
using AddressHash = std::unordered_set<h160>;

/// The zero address.
extern Address const ZeroAddress;

/// The last address.
extern Address const MaxAddress;

/// The SYSTEM address.
extern Address const SystemAddress;

}

