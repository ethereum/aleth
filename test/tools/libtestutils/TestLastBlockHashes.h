// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/** 
 * Test implementation for LastBlockHahsesFace
 */

#pragma once

#include <libethereum/LastBlockHashesFace.h>

namespace dev
{
namespace test
{

class TestLastBlockHashes: public eth::LastBlockHashesFace
{
public:
	explicit TestLastBlockHashes(h256s const& _hashes): m_hashes(_hashes) {}

	h256s precedingHashes(h256 const& /* _mostRecentHash */) const override { return m_hashes; }
	void clear() override {}

private:
	h256s const m_hashes;
};


}
}