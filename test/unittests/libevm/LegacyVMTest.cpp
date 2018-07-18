/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libethereum/LastBlockHashesFace.h>
#include <libevm/LegacyVM.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/jsontests/vm.h>
#include <boost/test/unit_test.hpp>

using namespace dev;
using namespace dev::test;
using namespace dev::eth;


namespace
{
class LastBlockHashes: public eth::LastBlockHashesFace
{
public:
    h256s precedingHashes(h256 const& /* _mostRecentHash */) const override
    {
        return h256s(256, h256());
    }
    void clear() override {}
};
}

BOOST_FIXTURE_TEST_SUITE(LegacyVMSuite, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(create2)
{
    BlockHeader blockHeader;
    blockHeader.setGasLimit(0x7fffffffffffffff);
    blockHeader.setTimestamp(0);

    LastBlockHashes lastBlockHashes;
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    Address address = KeyPair::create().address();
    State state(0);
    state.addBalance(address, 1 * ether);

    std::unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(eth::Network::ConstantinopleTest)).createSealEngine());


    u256 value = 0;
    u256 gasPrice = 1;
    int depth = 0;
    bool isCreate = true;
    bool staticCall = false;
    // mstore(0, 0x60)
    // return(0, 0x20)
    bytes inputData = fromHex("606060005260206000f3");
    // let s : = calldatasize()
    // calldatacopy(0, 0, s)
    // create2(0, 0, s, 0x123)
    // pop
    bytes code = fromHex("368060006000376101238160006000f55050");

    ExtVM extVm(state, envInfo, *se, address,
        address, address, value, gasPrice, ref(inputData),
        ref(code), sha3(code), depth, isCreate, staticCall);

    LegacyVM vm;

    u256 gas = 1000000;
    owning_bytes_ref res = vm.exec(gas, extVm, OnOpFunc{});

    Address expectedAddress = right160(sha3(address.asBytes() + toBigEndian(0x123_cppui256) + inputData));
    BOOST_REQUIRE(state.addressHasCode(expectedAddress));
}

BOOST_AUTO_TEST_SUITE_END()
