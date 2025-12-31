// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/**
 * The FixedHash fixed-size "hash" container type.
 */

#pragma once

#include "libdevcore/FixedHash.h"
#include "libdevcore/vector_ref.h"

namespace dev
{

h256 sha256(bytesConstRef _input) noexcept;

h160 ripemd160(bytesConstRef _input);

}
