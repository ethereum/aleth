/// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "EVMSchedule.h"
#include <libethcore/ChainOperationParams.h>

namespace dev
{
namespace eth
{
EVMSchedule addEIPs(EVMSchedule const& _schedule, AdditionalEIPs const& _eips)
{
    EVMSchedule modifiedSchedule = _schedule;
    if (_eips.eip2046)
        modifiedSchedule.precompileStaticCallGas = 40;

    return modifiedSchedule;
}
}  // namespace eth
}  // namespace dev
