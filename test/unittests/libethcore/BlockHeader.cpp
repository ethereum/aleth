// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libethcore/BlockHeader.h>

#include <gtest/gtest.h>

using namespace dev::eth;

TEST(BlockHeader, isValid)
{
    BlockHeader header;
    EXPECT_FALSE(header);
    header.setTimestamp(0);
    EXPECT_TRUE(header);
    header.setTimestamp(-2);
    EXPECT_FALSE(header);
}
