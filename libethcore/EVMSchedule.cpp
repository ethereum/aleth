/// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "EVMSchedule.h"
#include <libethcore/ChainOperationParams.h>

namespace dev
{
namespace eth
{
EVMSchedule::EVMSchedule(EVMSchedule const& _schedule, AdditionalEIPs const& _eips)
  : EVMSchedule(_schedule)
{
    if (_eips.eip1380)
        callSelfGas = 40;
    if (_eips.eip2046)
        precompileStaticCallGas = 40;
}
}  // namespace eth
}  // namespace dev
