// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// SealEngineFace class testing.
#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/TestHelper.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace
{
// Unsigned transaction is what eth_call RPC method uses for execution
class UnsignedTransactionFixture : public TestOutputHelperFixture
{
public:
    UnsignedTransactionFixture()
    {
        params.experimentalForkBlock = u256(0x1000);

        ethash.setChainParams(params);

        header.clear();
        header.setGasLimit(22000);
    }

    ChainOperationParams params;
    Ethash ethash;
    BlockHeader header;
    Transaction tx{0, 0, 21000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0};
};
}  // namespace

BOOST_FIXTURE_TEST_SUITE(SealEngineTests, TestOutputHelperFixture)
BOOST_FIXTURE_TEST_SUITE(UnsignedTransactionTests, UnsignedTransactionFixture)

BOOST_AUTO_TEST_CASE(UnsignedTransactionIsValidBeforeExperimental)
{
    // Unsigned transaction (having empty optional for signature)  should be valid otherwise
    // eth_call would fail All transactions coming from the network or RPC methods like
    // sendRawTransaction have their optional<Signature> initialized,
    // so these tests don't cover them

    header.setNumber(1);

    ethash.SealEngineFace::verifyTransaction(
        ImportRequirements::TransactionSignatures, tx, header, 0);  // check that it doesn't throw
}

BOOST_AUTO_TEST_CASE(UnsignedTransactionIsValidInExperimental)
{
    header.setNumber(0x1010);

    ethash.SealEngineFace::verifyTransaction(
        ImportRequirements::TransactionSignatures, tx, header, 0);  // check that it doesn't throw
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
