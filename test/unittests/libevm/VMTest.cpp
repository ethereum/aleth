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

#include <libaleth-interpreter/interpreter.h>
#include <libethereum/LastBlockHashesFace.h>
#include <libevm/EVMC.h>
#include <libevm/LegacyVM.h>
#include <test/tools/jsontests/vm.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

using namespace dev;
using namespace dev::test;
using namespace dev::eth;


namespace
{
class LastBlockHashes : public eth::LastBlockHashesFace
{
public:
    h256s precedingHashes(h256 const& /* _mostRecentHash */) const override
    {
        return h256s(256, h256());
    }
    void clear() override {}
};

BlockHeader initBlockHeader()
{
    BlockHeader blockHeader;
    blockHeader.setGasLimit(0x7fffffffffffffff);
    blockHeader.setTimestamp(0);
    return blockHeader;
}

class Create2TestFixture: public TestOutputHelperFixture
{
public:
    explicit Create2TestFixture(VMFace* _vm): vm{_vm} { state.addBalance(address, 1 * ether); }

    void testCreate2worksInConstantinople()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE(state.addressHasCode(expectedAddress));
    }

    void testCreate2isInvalidBeforeConstantinople()
    {
        se.reset(ChainParams(genesisInfo(Network::ByzantiumTest)).createSealEngine());

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        BOOST_REQUIRE_THROW(vm->exec(gas, extVm, OnOpFunc{}), BadInstruction);
    }

    void testCreate2succeedsIfAddressHasEther()
    {
        state.addBalance(expectedAddress, 1 * ether);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE(state.addressHasCode(expectedAddress));
    }

    void testCreate2doesntChangeContractIfAddressExists()
    {
        state.setCode(expectedAddress, bytes{inputData}, 0);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        vm->exec(gas, extVm, OnOpFunc{});
        BOOST_REQUIRE(state.code(expectedAddress) == inputData);
    }

    void testCreate2isForbiddenInStaticCall()
    {
        staticCall = true;

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        BOOST_REQUIRE_THROW(vm->exec(gas, extVm, OnOpFunc{}), DisallowedStateChange);
    }

    void testCreate2collisionWithNonEmptyStorage()
    {
        // Theoretical edge-case for an account with empty code and zero nonce and balance and
        // non-empty storage. This account should be considered empty and CREATE2 over should be
        // able to overwrite it and clear storage.
        state.createContract(expectedAddress);
        state.setStorage(expectedAddress, 1, 1);
        state.commit(State::CommitBehaviour::KeepEmptyAccounts);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        vm->exec(gas, extVm, OnOpFunc{});
        BOOST_REQUIRE(state.addressHasCode(expectedAddress));
        BOOST_REQUIRE_EQUAL(state.storage(expectedAddress, 1), 0);
        BOOST_REQUIRE_EQUAL(state.getNonce(expectedAddress), 1);
    }

    void testCreate2collisionWithNonEmptyStorageEmptyInitCode()
    {
        // Similar to previous case but with empty init code
        inputData.clear();
        expectedAddress = right160(sha3(
            fromHex("ff") + address.asBytes() + toBigEndian(0x123_cppui256) + sha3(inputData)));

        state.createContract(expectedAddress);
        state.setStorage(expectedAddress, 1, 1);
        state.commit(State::CommitBehaviour::KeepEmptyAccounts);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        vm->exec(gas, extVm, OnOpFunc{});
        BOOST_REQUIRE_EQUAL(state.storage(expectedAddress, 1), 0);
        BOOST_REQUIRE_EQUAL(state.getNonce(expectedAddress), 1);
    }

    void testCreate2costIncludesInitCodeHashing()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, ref(inputData),
            ref(code), sha3(code), version, depth, isCreate, staticCall);

        uint64_t gasBefore = 0;
        uint64_t gasAfter = 0;
        auto onOp = [&gasBefore, &gasAfter](uint64_t /*steps*/, uint64_t /* PC */,
                        Instruction _instr, bigint /* newMemSize */, bigint /* gasCost */,
                        bigint _gas, VMFace const*, ExtVMFace const*) {
            if (_instr == Instruction::CREATE2)
            {
                // before CREATE2 instruction
                gasBefore = static_cast<uint64_t>(_gas);
            }
            else if (gasBefore != 0 && gasAfter == 0)
            {
                // first instruction of the init code
                gasAfter = static_cast<uint64_t>(_gas);
            }
        };

        vm->exec(gas, extVm, onOp);

        // create cost
        uint64_t expectedGasAfter = gasBefore - 32000;
        // hashing cost, assuming no memory expansion needed
        expectedGasAfter -=
            static_cast<uint64_t>(std::ceil(static_cast<double>(inputData.size()) / 32)) * 6;
        // EIP-150 adjustion of subcall gas
        expectedGasAfter -= expectedGasAfter / 64;
        BOOST_REQUIRE_EQUAL(gasAfter, expectedGasAfter);
    }


    BlockHeader blockHeader{initBlockHeader()};
    LastBlockHashes lastBlockHashes;
    Address address{KeyPair::create().address()};
    State state{0};
    std::unique_ptr<SealEngineFace> se{
        ChainParams(genesisInfo(Network::ConstantinopleTest)).createSealEngine()};
    EnvInfo envInfo{blockHeader, lastBlockHashes, 0, se->chainParams().chainID};

    u256 value = 0;
    u256 gasPrice = 1;
    u256 version = 0;
    int depth = 0;
    bool isCreate = false;
    bool staticCall = false;
    u256 gas = 1000000;

    // mstore(0, 0x60)
    // return(0, 0x20)
    bytes inputData = fromHex("606060005260206000f3");

    // let s : = calldatasize()
    // calldatacopy(0, 0, s)
    // create2(0, 0, s, 0x123)
    // pop
    bytes code = fromHex("368060006000376101238160006000f55050");

    Address expectedAddress = right160(
        sha3(fromHex("ff") + address.asBytes() + toBigEndian(0x123_cppui256) + sha3(inputData)));

    std::unique_ptr<VMFace> vm;
};

class LegacyVMCreate2TestFixture: public Create2TestFixture
{
public:
    LegacyVMCreate2TestFixture(): Create2TestFixture{new LegacyVM} {}
};

class AlethInterpreterCreate2TestFixture: public Create2TestFixture
{
public:
    AlethInterpreterCreate2TestFixture(): Create2TestFixture{new EVMC{evmc_create_interpreter()}} {}
};

class ExtcodehashTestFixture : public TestOutputHelperFixture
{
public:
    explicit ExtcodehashTestFixture(VMFace* _vm) : vm{_vm}
    {
        state.addBalance(address, 1 * ether);
        state.setCode(extAddress, bytes{extCode}, 0);
    }

    void testExtcodehashWorksInConstantinople()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            extAddress.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE(ret.toBytes() == sha3(extCode).asBytes());
    }

    void testExtcodehashHasCorrectCost()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            extAddress.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        bigint gasBefore;
        bigint gasAfter;
        auto onOp = [&gasBefore, &gasAfter](uint64_t /*steps*/, uint64_t /* PC */,
                        Instruction _instr, bigint /*newMemSize*/, bigint /*gasCost*/, bigint _gas,
                        VMFace const*, ExtVMFace const*) {
            if (_instr == Instruction::EXTCODEHASH)
                gasBefore = _gas;
            else if (gasBefore != 0 && gasAfter == 0)
                gasAfter = _gas;
        };

        vm->exec(gas, extVm, onOp);

        BOOST_REQUIRE_EQUAL(gasBefore - gasAfter, 400);
    }

    void testExtCodeHashisInvalidBeforeConstantinople()
    {
        se.reset(ChainParams(genesisInfo(Network::ByzantiumTest)).createSealEngine());

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            extAddress.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        BOOST_REQUIRE_THROW(vm->exec(gas, extVm, OnOpFunc{}), BadInstruction);
    }

    void testExtCodeHashOfNonContractAccount()
    {
        Address addressWithEmptyCode{KeyPair::create().address()};
        state.addBalance(addressWithEmptyCode, 1 * ether);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            addressWithEmptyCode.ref(), ref(code), sha3(code), version, depth, isCreate,
            staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(toHex(ret.toBytes()),
            "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
    }

    void testExtCodeHashOfNonExistentAccount()
    {
        Address addressNonExisting{0x1234};

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            addressNonExisting.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(fromBigEndian<int>(ret.toBytes()), 0);
    }

    void testExtCodeHashOfPrecomileZeroBalance()
    {
        Address addressPrecompile{0x1};

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            addressPrecompile.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(fromBigEndian<int>(ret.toBytes()), 0);
    }

    void testExtCodeHashOfPrecomileNonZeroBalance()
    {
        Address addressPrecompile{0x1};
        state.addBalance(addressPrecompile, 1 * ether);

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            addressPrecompile.ref(), ref(code), sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(toHex(ret.toBytes()),
            "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
    }

    void testExtcodehashIgnoresHigh12Bytes()
    {
        // calldatacopy(0, 0, 32)
        // let addr : = mload(0)
        // let hash : = extcodehash(addr)
        // mstore(0, hash)
        // return(0, 32)
        code = fromHex("60206000600037600051803f8060005260206000f35050");

        bytes extAddressPrefixed =
            bytes{1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc} + extAddress.ref();

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice,
            ref(extAddressPrefixed), ref(code), sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE(ret.toBytes() == sha3(extCode).asBytes());
    }

    BlockHeader blockHeader{initBlockHeader()};
    LastBlockHashes lastBlockHashes;
    Address address{KeyPair::create().address()};
    Address extAddress{KeyPair::create().address()};
    State state{0};
    std::unique_ptr<SealEngineFace> se{
        ChainParams(genesisInfo(Network::ConstantinopleTest)).createSealEngine()};
    EnvInfo envInfo{blockHeader, lastBlockHashes, 0, se->chainParams().chainID};

    u256 value = 0;
    u256 gasPrice = 1;
    u256 version = 0;
    int depth = 0;
    bool isCreate = false;
    bool staticCall = false;
    u256 gas = 1000000;

    // mstore(0, 0x60)
    // return(0, 0x20)
    bytes extCode = fromHex("606060005260206000f3");

    // calldatacopy(12, 0, 20)
    // let addr : = mload(0)
    // let hash : = extcodehash(addr)
    // mstore(0, hash)
    // return(0, 32)
    bytes code = fromHex("60146000600c37600051803f8060005260206000f35050");

    std::unique_ptr<VMFace> vm;
};

class LegacyVMExtcodehashTestFixture : public ExtcodehashTestFixture
{
public:
    LegacyVMExtcodehashTestFixture() : ExtcodehashTestFixture{new LegacyVM} {}
};


class AlethInterpreterExtcodehashTestFixture : public ExtcodehashTestFixture
{
public:
    AlethInterpreterExtcodehashTestFixture()
      : ExtcodehashTestFixture{new EVMC{evmc_create_interpreter()}}
    {}
};

class SstoreTestFixture : public TestOutputHelperFixture
{
public:
    struct Case
    {
        char const* code;
        u256 const originalValue;
        u256 const expectedGasConsumed;
        u256 const expectedRefund;
    };

    // clang-format off
    std::array<Case, 17> const constantinopleCases = {{
        { "0x60006000556000600055",           0, 412,   0     },
        { "0x60006000556001600055",           0, 20212, 0     },
        { "0x60016000556000600055",           0, 20212, 19800 },
        { "0x60016000556002600055",           0, 20212, 0     },
        { "0x60016000556001600055",           0, 20212, 0     },
        { "0x60006000556000600055",           1, 5212,  15000 },
        { "0x60006000556001600055",           1, 5212,  4800  },
        { "0x60006000556002600055",           1, 5212,  0     },
        { "0x60026000556000600055",           1, 5212,  15000 },
        { "0x60026000556003600055",           1, 5212,  0     },
        { "0x60026000556001600055",           1, 5212,  4800  },
        { "0x60026000556002600055",           1, 5212,  0     },
        { "0x60016000556000600055",           1, 5212,  15000 },
        { "0x60016000556002600055",           1, 5212,  0     },
        { "0x60016000556001600055",           1, 412,   0     },
        { "0x600160005560006000556001600055", 0, 40218, 19800 },
        { "0x600060005560016000556000600055", 1, 10218, 19800 },
    }};

    std::array<Case, 17> const petersburgCases = {{
        { "0x60006000556000600055",           0, 10012, 0     },
        { "0x60006000556001600055",           0, 25012, 0     },
        { "0x60016000556000600055",           0, 25012, 15000 },
        { "0x60016000556002600055",           0, 25012, 0     },
        { "0x60016000556001600055",           0, 25012, 0     },
        { "0x60006000556000600055",           1, 10012, 15000 },
        { "0x60006000556001600055",           1, 25012, 15000 },
        { "0x60006000556002600055",           1, 25012, 15000 },
        { "0x60026000556000600055",           1, 10012, 15000 },
        { "0x60026000556003600055",           1, 10012, 0     },
        { "0x60026000556001600055",           1, 10012, 0     },
        { "0x60026000556002600055",           1, 10012, 0     },
        { "0x60016000556000600055",           1, 10012, 15000 },
        { "0x60016000556002600055",           1, 10012, 0     },
        { "0x60016000556001600055",           1, 10012, 0     },
        { "0x600160005560006000556001600055", 0, 45018, 15000 },
        { "0x600060005560016000556000600055", 1, 30018, 30000 },
    }};

    std::array<Case, 17> const istanbulCases = {{
        { "0x60006000556000600055",           0, 1612,  0     },
        { "0x60006000556001600055",           0, 20812, 0     },
        { "0x60016000556000600055",           0, 20812, 19200 },
        { "0x60016000556002600055",           0, 20812, 0     },
        { "0x60016000556001600055",           0, 20812, 0     },
        { "0x60006000556000600055",           1, 5812,  15000 },
        { "0x60006000556001600055",           1, 5812,  4200  },
        { "0x60006000556002600055",           1, 5812,  0     },
        { "0x60026000556000600055",           1, 5812,  15000 },
        { "0x60026000556003600055",           1, 5812,  0     },
        { "0x60026000556001600055",           1, 5812,  4200  },
        { "0x60026000556002600055",           1, 5812,  0     },
        { "0x60016000556000600055",           1, 5812,  15000 },
        { "0x60016000556002600055",           1, 5812,  0     },
        { "0x60016000556001600055",           1, 1612,  0    },
        { "0x600160005560006000556001600055", 0, 40818, 19200 },
        { "0x600060005560016000556000600055", 1, 10818, 19200 },
    }};
    // clang-format on

    explicit SstoreTestFixture(VMFace* _vm) : vm{_vm}
    {
        state.addBalance(from, 1 * ether);
        state.addBalance(to, 1 * ether);
    }

    void testGasConsumed(SealEngineFace const& _se, EnvInfo const& _envInfo, std::string const& _codeStr,
        u256 const& _originalValue, u256 const& _expectedGasConsumed, u256 const& _expectedRefund)
    {
        state.setStorage(to, 0, _originalValue);
        state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

        bytes const code = fromHex(_codeStr);
        ExtVM extVm(state, _envInfo, _se, to, from, from, value, gasPrice, inputData, ref(code),
            sha3(code), version, depth, isCreate, staticCall);

        u256 gasBefore = gas;
        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_CHECK_MESSAGE(gasBefore - gas == _expectedGasConsumed,
            boost::format("code: %1%, gas consumed: %2%, gas expected: %3%") % _codeStr %
                (gasBefore - gas) % _expectedGasConsumed);
        BOOST_CHECK_MESSAGE(extVm.sub.refunds == _expectedRefund,
            boost::format("code: %1%, refund: %2%, refund expected: %3%") % _codeStr %
                extVm.sub.refunds % _expectedRefund);
    }


    void testAllCases(std::array<Case, 17> const& _cases, Network _network)
    {
        std::unique_ptr<SealEngineFace> se{ChainParams(genesisInfo(_network)).createSealEngine()};
        EnvInfo envInfo{blockHeader, lastBlockHashes, 0, se->chainParams().chainID};

        for (auto const& c : _cases)
            testGasConsumed(*se, envInfo, c.code, c.originalValue, c.expectedGasConsumed, c.expectedRefund);
    }

    void testConstantinople() { testAllCases(constantinopleCases, Network::ConstantinopleTest); }

    void testPetersburg() { testAllCases(petersburgCases, Network::ConstantinopleFixTest); }

    void testIstanbul() { testAllCases(istanbulCases, Network::IstanbulTest); }

    BlockHeader blockHeader{initBlockHeader()};
    LastBlockHashes lastBlockHashes;
    Address from{KeyPair::create().address()};
    Address to{KeyPair::create().address()};
    State state{0};

    u256 value = 0;
    u256 gasPrice = 1;
    u256 version = 0;
    int depth = 0;
    bool isCreate = false;
    bool staticCall = false;
    u256 gas = 1000000;
    bytesConstRef inputData;

    std::unique_ptr<VMFace> vm;
};

class LegacyVMSstoreTestFixture : public SstoreTestFixture
{
public:
    LegacyVMSstoreTestFixture() : SstoreTestFixture{new LegacyVM} {}
};

class AlethInterpreterSstoreTestFixture : public SstoreTestFixture
{
public:
    AlethInterpreterSstoreTestFixture() : SstoreTestFixture{new EVMC{evmc_create_interpreter()}} {}
};

class ChainIDTestFixture : public TestOutputHelperFixture
{
public:
    explicit ChainIDTestFixture(VMFace* _vm) : vm{_vm} { state.addBalance(address, 1 * ether); }

    void testChainIDWorksInIstanbul()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {}, ref(code),
            sha3(code), version, depth, isCreate, staticCall);

        owning_bytes_ref ret = vm->exec(gas, extVm, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(fromBigEndian<int>(ret), 1);
    }

    void testChainIDHasCorrectCost()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {}, ref(code),
            sha3(code), version, depth, isCreate, staticCall);

        bigint gasBefore;
        bigint gasAfter;
        auto onOp = [&gasBefore, &gasAfter](uint64_t /*steps*/, uint64_t /* PC */,
                        Instruction _instr, bigint /*newMemSize*/, bigint /*gasCost*/, bigint _gas,
                        VMFace const*, ExtVMFace const*) {
            if (_instr == Instruction::CHAINID)
                gasBefore = _gas;
            else if (gasBefore != 0 && gasAfter == 0)
                gasAfter = _gas;
        };

        vm->exec(gas, extVm, onOp);

        BOOST_REQUIRE_EQUAL(gasBefore - gasAfter, 2);
    }

    void testChainIDisInvalidBeforeIstanbul()
    {
        se.reset(ChainParams(genesisInfo(Network::ConstantinopleFixTest)).createSealEngine());
        version = ConstantinopleFixSchedule.accountVersion;

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {}, ref(code),
            sha3(code), version, depth, isCreate, staticCall);

        BOOST_REQUIRE_THROW(vm->exec(gas, extVm, OnOpFunc{}), BadInstruction);
    }


    BlockHeader blockHeader{initBlockHeader()};
    LastBlockHashes lastBlockHashes;
    Address address{KeyPair::create().address()};
    State state{0};
    std::unique_ptr<SealEngineFace> se{
        ChainParams(genesisInfo(Network::IstanbulTest)).createSealEngine()};
    EnvInfo envInfo{blockHeader, lastBlockHashes, 0, se->chainParams().chainID};

    u256 value = 0;
    u256 gasPrice = 1;
    u256 version = IstanbulSchedule.accountVersion;
    int depth = 0;
    bool isCreate = false;
    bool staticCall = false;
    u256 gas = 1000000;

    // let id : = chainid()
    // mstore(0, id)
    // return(0, 32)
    bytes code = fromHex("468060005260206000f350");

    std::unique_ptr<VMFace> vm;
};

class LegacyVMChainIDTestFixture : public ChainIDTestFixture
{
public:
    LegacyVMChainIDTestFixture() : ChainIDTestFixture{new LegacyVM} {}
};

class AlethInterpreterChainIDTestFixture : public ChainIDTestFixture
{
public:
    AlethInterpreterChainIDTestFixture() : ChainIDTestFixture{new EVMC{evmc_create_interpreter()}}
    {}
};

class BalanceFixture : public TestOutputHelperFixture
{
public:
    explicit BalanceFixture(VMFace* _vm) : vm{_vm} { state.addBalance(address, 1 * ether); }

    void testSelfBalanceWorksInIstanbul()
    {
        ExtVM extVmSelfBalance(state, envInfo, *se, address, address, address, value, gasPrice, {},
            ref(codeSelfBalance), sha3(codeSelfBalance), version, depth, isCreate, staticCall);

        owning_bytes_ref retSelfBalance = vm->exec(gas, extVmSelfBalance, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(fromBigEndian<u256>(retSelfBalance), 1 * ether);

        ExtVM extVmBalance(state, envInfo, *se, address, address, address, value, gasPrice, {},
            ref(codeBalance), sha3(codeBalance), version, depth, isCreate, staticCall);

        owning_bytes_ref retBalance = vm->exec(gas, extVmBalance, OnOpFunc{});

        BOOST_REQUIRE_EQUAL(fromBigEndian<u256>(retBalance), fromBigEndian<u256>(retSelfBalance));
    }

    void testSelfBalanceHasCorrectCost()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {},
            ref(codeSelfBalance), sha3(codeSelfBalance), version, depth, isCreate, staticCall);

        bigint gasBefore;
        bigint gasAfter;
        auto onOp = [&gasBefore, &gasAfter](uint64_t /*steps*/, uint64_t /* PC */,
                        Instruction _instr, bigint /*newMemSize*/, bigint /*gasCost*/, bigint _gas,
                        VMFace const*, ExtVMFace const*) {
            if (_instr == Instruction::SELFBALANCE)
                gasBefore = _gas;
            else if (gasBefore != 0 && gasAfter == 0)
                gasAfter = _gas;
        };

        vm->exec(gas, extVm, onOp);

        BOOST_REQUIRE_EQUAL(gasBefore - gasAfter, 5);
    }

    void testBalanceHasCorrectCost()
    {
        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {},
            ref(codeBalance), sha3(codeBalance), version, depth, isCreate, staticCall);

        bigint gasBefore;
        bigint gasAfter;
        auto onOp = [&gasBefore, &gasAfter](uint64_t /*steps*/, uint64_t /* PC */,
                        Instruction _instr, bigint /*newMemSize*/, bigint /*gasCost*/, bigint _gas,
                        VMFace const*, ExtVMFace const*) {
            if (_instr == Instruction::BALANCE)
                gasBefore = _gas;
            else if (gasBefore != 0 && gasAfter == 0)
                gasAfter = _gas;
        };

        vm->exec(gas, extVm, onOp);

        BOOST_REQUIRE_EQUAL(gasBefore - gasAfter, 700);
    }

    void testSelfBalanceisInvalidBeforeIstanbul()
    {
        se.reset(ChainParams(genesisInfo(Network::ConstantinopleFixTest)).createSealEngine());
        version = ConstantinopleFixSchedule.accountVersion;

        ExtVM extVm(state, envInfo, *se, address, address, address, value, gasPrice, {},
            ref(codeSelfBalance), sha3(codeSelfBalance), version, depth, isCreate, staticCall);

        BOOST_REQUIRE_THROW(vm->exec(gas, extVm, OnOpFunc{}), BadInstruction);
    }


    BlockHeader blockHeader{initBlockHeader()};
    LastBlockHashes lastBlockHashes;
    Address address{KeyPair::create().address()};
    State state{0};
    std::unique_ptr<SealEngineFace> se{
        ChainParams(genesisInfo(Network::IstanbulTest)).createSealEngine()};
    EnvInfo envInfo{blockHeader, lastBlockHashes, 0, se->chainParams().chainID};

    u256 value = 0;
    u256 gasPrice = 1;
    u256 version = IstanbulSchedule.accountVersion;
    int depth = 0;
    bool isCreate = false;
    bool staticCall = false;
    u256 gas = 1000000;

    // let b : = selfbalance()
    // mstore(0, b)
    // return(0, 32)
    bytes codeSelfBalance = fromHex("478060005260206000f350");

    // let a := caller()
    // let b := balance(a)
    // mstore(0, b)
    // return(0, 32)
    bytes codeBalance = fromHex("3380318060005260206000f35050");

    std::unique_ptr<VMFace> vm;
};

class LegacyVMBalanceFixture : public BalanceFixture
{
public:
    LegacyVMBalanceFixture() : BalanceFixture{new LegacyVM} {}
};

class AlethInterpreterBalanceFixture : public BalanceFixture
{
public:
    AlethInterpreterBalanceFixture() : BalanceFixture{new EVMC{evmc_create_interpreter()}} {}
};
}  // namespace

BOOST_FIXTURE_TEST_SUITE(LegacyVMSuite, TestOutputHelperFixture)
BOOST_FIXTURE_TEST_SUITE(LegacyVMCreate2Suite, LegacyVMCreate2TestFixture)

BOOST_AUTO_TEST_CASE(LegacyVMCreate2worksInConstantinople)
{
    testCreate2worksInConstantinople();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2isInvalidBeforeConstantinople)
{
    testCreate2isInvalidBeforeConstantinople();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2succeedsIfAddressHasEther)
{
    testCreate2succeedsIfAddressHasEther();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2doesntChangeContractIfAddressExists)
{
    testCreate2doesntChangeContractIfAddressExists();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2isForbiddenInStaticCall)
{
    testCreate2isForbiddenInStaticCall();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2collisionWithNonEmptyStorage)
{
    testCreate2collisionWithNonEmptyStorage();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2collisionWithNonEmptyStorageEmptyInitCode)
{
    testCreate2collisionWithNonEmptyStorageEmptyInitCode();
}

BOOST_AUTO_TEST_CASE(LegacyVMCreate2costIncludesInitCodeHashing)
{
    testCreate2costIncludesInitCodeHashing();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(LegacyVMExtcodehashSuite, LegacyVMExtcodehashTestFixture)

BOOST_AUTO_TEST_CASE(LegacyVMExtcodehashWorksInConstantinople)
{
    testExtcodehashWorksInConstantinople();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtcodehashHasCorrectCost)
{
    testExtcodehashHasCorrectCost();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtcodehashIsInvalidConstantinople)
{
    testExtCodeHashisInvalidBeforeConstantinople();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtCodeHashOfNonContractAccount)
{
    testExtCodeHashOfNonContractAccount();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtCodeHashOfNonExistentAccount)
{
    testExtCodeHashOfPrecomileZeroBalance();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtCodeHashOfPrecomileZeroBalance)
{
    testExtCodeHashOfNonExistentAccount();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtCodeHashOfPrecomileNonZeroBalance)
{
    testExtCodeHashOfPrecomileNonZeroBalance();
}

BOOST_AUTO_TEST_CASE(LegacyVMExtcodehashIgnoresHigh12Bytes)
{
    testExtcodehashIgnoresHigh12Bytes();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(LegacyVMSstoreSuite, LegacyVMSstoreTestFixture)

BOOST_AUTO_TEST_CASE(LegacyVMSstoreConstantinople)
{
    testConstantinople();
}

BOOST_AUTO_TEST_CASE(LegacyVMSstorePetersburg)
{
    testPetersburg();
}

BOOST_AUTO_TEST_CASE(LegacyVMSstoreIstanbul)
{
    testIstanbul();
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(LegacyVMChainIDSuite, LegacyVMChainIDTestFixture)

BOOST_AUTO_TEST_CASE(LegacyVMChainIDworksInIstanbul)
{
    testChainIDWorksInIstanbul();
}

BOOST_AUTO_TEST_CASE(LegacyVMChainIDHasCorrectCost)
{
    testChainIDHasCorrectCost();
}

BOOST_AUTO_TEST_CASE(LegacyVMChainIDisInvalidBeforeIstanbul)
{
    testChainIDisInvalidBeforeIstanbul();
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(LegacyVMBalanceSuite, LegacyVMBalanceFixture)

BOOST_AUTO_TEST_CASE(LegacyVMSelfBalanceworksInIstanbul)
{
    testSelfBalanceWorksInIstanbul();
}

BOOST_AUTO_TEST_CASE(LegacyVMSelfBalanceHasCorrectCost)
{
    testSelfBalanceHasCorrectCost();
}

BOOST_AUTO_TEST_CASE(LegacyVMBalanceHasCorrectCost)
{
    testBalanceHasCorrectCost();
}

BOOST_AUTO_TEST_CASE(LegacyVMSelfBalanceisInvalidBeforeIstanbul)
{
    testSelfBalanceisInvalidBeforeIstanbul();
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(AlethInterpreterSuite, TestOutputHelperFixture)
BOOST_FIXTURE_TEST_SUITE(AlethInterpreterCreate2Suite, AlethInterpreterCreate2TestFixture)

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2worksInConstantinople)
{
    testCreate2worksInConstantinople();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2isInvalidBeforeConstantinople)
{
    testCreate2isInvalidBeforeConstantinople();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2succeedsIfAddressHasEther)
{
    testCreate2succeedsIfAddressHasEther();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2doesntChangeContractIfAddressExists)
{
    testCreate2doesntChangeContractIfAddressExists();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2isForbiddenInStaticCall)
{
    testCreate2isForbiddenInStaticCall();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2collisionWithNonEmptyStorage)
{
    testCreate2collisionWithNonEmptyStorage();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterCreate2collisionWithNonEmptyStorageEmptyInitCode)
{
    testCreate2collisionWithNonEmptyStorageEmptyInitCode();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(AlethInterpreterExtcodehashSuite, AlethInterpreterExtcodehashTestFixture)

BOOST_AUTO_TEST_CASE(AlethInterpreterExtcodehashWorksInConstantinople)
{
    testExtcodehashWorksInConstantinople();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtcodehashIsInvalidConstantinople)
{
    testExtCodeHashisInvalidBeforeConstantinople();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtCodeHashOfNonContractAccount)
{
    testExtCodeHashOfNonContractAccount();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtCodeHashOfNonExistentAccount)
{
    testExtCodeHashOfPrecomileZeroBalance();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtCodeHashOfPrecomileZeroBalance)
{
    testExtCodeHashOfNonExistentAccount();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtCodeHashOfPrecomileNonZeroBalance)
{
    testExtCodeHashOfPrecomileNonZeroBalance();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterExtCodeHashIgnoresHigh12Bytes)
{
    testExtcodehashIgnoresHigh12Bytes();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(AlethInterpreterSstoreSuite, AlethInterpreterSstoreTestFixture)

BOOST_AUTO_TEST_CASE(AlethInterpreterSstoreConstantinople)
{
    testConstantinople();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterSstorePetersburg)
{
    testPetersburg();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(AlethInterpreterChainIDSuite, AlethInterpreterChainIDTestFixture)

BOOST_AUTO_TEST_CASE(AlethInterpreterChainIDworksInIstanbul)
{
    testChainIDWorksInIstanbul();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterChainIDisInvalidBeforeIstanbul)
{
    testChainIDisInvalidBeforeIstanbul();
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(AlethInterpreterBalanceSuite, AlethInterpreterBalanceFixture)

BOOST_AUTO_TEST_CASE(AlethInterpreterSelfBalanceworksInIstanbul)
{
    testSelfBalanceWorksInIstanbul();
}

BOOST_AUTO_TEST_CASE(AlethInterpreterSelfBalanceisInvalidBeforeIstanbul)
{
    testSelfBalanceisInvalidBeforeIstanbul();
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
