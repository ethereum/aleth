// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "vector_ref.h"

namespace dev
{
namespace db
{
using Slice = vector_ref<char const>;
class WriteBatchFace;
class DatabaseFace;
}
}
