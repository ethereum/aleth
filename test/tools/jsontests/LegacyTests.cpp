// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <test/tools/jsontests/LegacyTests.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <boost/test/unit_test.hpp>
using namespace dev::test;

BOOST_AUTO_TEST_SUITE(LegacyTests)
BOOST_AUTO_TEST_SUITE(Constantinople)
BOOST_FIXTURE_TEST_SUITE(GeneralStateTests, LegacyConstantinopleGeneralStateTestFixture)
// Frontier Tests
BOOST_AUTO_TEST_CASE(stCallCodes) {}
BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest) {}
BOOST_AUTO_TEST_CASE(stExample) {}
BOOST_AUTO_TEST_CASE(stInitCodeTest) {}
BOOST_AUTO_TEST_CASE(stLogTests) {}
BOOST_AUTO_TEST_CASE(stMemoryTest) {}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts) {}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts2) {}
BOOST_AUTO_TEST_CASE(stRandom) {}
BOOST_AUTO_TEST_CASE(stRandom2) {}
BOOST_AUTO_TEST_CASE(stRecursiveCreate) {}
BOOST_AUTO_TEST_CASE(stRefundTest) {}
BOOST_AUTO_TEST_CASE(stSolidityTest) {}
BOOST_AUTO_TEST_CASE(stSpecialTest) {}
BOOST_AUTO_TEST_CASE(stSystemOperationsTest) {}
BOOST_AUTO_TEST_CASE(stTransactionTest) {}
BOOST_AUTO_TEST_CASE(stTransitionTest) {}
BOOST_AUTO_TEST_CASE(stWalletTest) {}

// Homestead Tests
BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead) {}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead) {}
BOOST_AUTO_TEST_CASE(stHomesteadSpecific) {}
BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead) {}

// EIP150 Tests
BOOST_AUTO_TEST_CASE(stChangedEIP150) {}
BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices) {}
BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls) {}
BOOST_AUTO_TEST_CASE(stEIP150Specific) {}

// EIP158 Tests
BOOST_AUTO_TEST_CASE(stEIP158Specific) {}
BOOST_AUTO_TEST_CASE(stNonZeroCallsTest) {}
BOOST_AUTO_TEST_CASE(stZeroCallsTest) {}
BOOST_AUTO_TEST_CASE(stZeroCallsRevert) {}
BOOST_AUTO_TEST_CASE(stCodeSizeLimit) {}
BOOST_AUTO_TEST_CASE(stCreateTest) {}
BOOST_AUTO_TEST_CASE(stRevertTest) {}

// Metropolis Tests
BOOST_AUTO_TEST_CASE(stStackTests) {}
BOOST_AUTO_TEST_CASE(stStaticCall) {}
BOOST_AUTO_TEST_CASE(stReturnDataTest) {}
BOOST_AUTO_TEST_CASE(stZeroKnowledge) {}
BOOST_AUTO_TEST_CASE(stZeroKnowledge2) {}
BOOST_AUTO_TEST_CASE(stCodeCopyTest) {}
BOOST_AUTO_TEST_CASE(stBugs) {}

// Constantinople Tests
BOOST_AUTO_TEST_CASE(stShift) {}
BOOST_AUTO_TEST_CASE(stCreate2) {}
BOOST_AUTO_TEST_CASE(stExtCodeHash) {}
BOOST_AUTO_TEST_CASE(stSStoreTest) {}

// Stress Tests
BOOST_AUTO_TEST_CASE(stAttackTest) {}
BOOST_AUTO_TEST_CASE(stMemoryStressTest) {}
BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest) {}

// Invalid Opcode Tests
BOOST_AUTO_TEST_CASE(stBadOpcode) {}

// New Tests
BOOST_AUTO_TEST_CASE(stArgsZeroOneBalance) {}
BOOST_AUTO_TEST_CASE(stEWASMTests) {}
BOOST_AUTO_TEST_CASE(stTimeConsuming) {}
BOOST_AUTO_TEST_SUITE_END()  // GeneralStateTests Constantinople Legacy

BOOST_FIXTURE_TEST_SUITE(BCGeneralStateTests, LegacyConstantinopleBCGeneralStateTestFixture)
// Frontier Tests
BOOST_AUTO_TEST_CASE(stCallCodes) {}
BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest) {}
BOOST_AUTO_TEST_CASE(stExample) {}
BOOST_AUTO_TEST_CASE(stInitCodeTest) {}
BOOST_AUTO_TEST_CASE(stLogTests) {}
BOOST_AUTO_TEST_CASE(stMemoryTest) {}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts) {}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts2) {}
BOOST_AUTO_TEST_CASE(stRandom) {}
BOOST_AUTO_TEST_CASE(stRandom2) {}
BOOST_AUTO_TEST_CASE(stRecursiveCreate) {}
BOOST_AUTO_TEST_CASE(stRefundTest) {}
BOOST_AUTO_TEST_CASE(stSolidityTest) {}
BOOST_AUTO_TEST_CASE(stSpecialTest) {}
BOOST_AUTO_TEST_CASE(stSystemOperationsTest) {}
BOOST_AUTO_TEST_CASE(stTransactionTest) {}
BOOST_AUTO_TEST_CASE(stTransitionTest) {}
BOOST_AUTO_TEST_CASE(stWalletTest) {}

// Homestead Tests
BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead) {}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead) {}
BOOST_AUTO_TEST_CASE(stHomesteadSpecific) {}
BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead) {}

// EIP150 Tests
BOOST_AUTO_TEST_CASE(stChangedEIP150) {}
BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices) {}
BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls) {}
BOOST_AUTO_TEST_CASE(stEIP150Specific) {}

// EIP158 Tests
BOOST_AUTO_TEST_CASE(stEIP158Specific) {}
BOOST_AUTO_TEST_CASE(stNonZeroCallsTest) {}
BOOST_AUTO_TEST_CASE(stZeroCallsTest) {}
BOOST_AUTO_TEST_CASE(stZeroCallsRevert) {}
BOOST_AUTO_TEST_CASE(stCodeSizeLimit) {}
BOOST_AUTO_TEST_CASE(stCreateTest) {}
BOOST_AUTO_TEST_CASE(stRevertTest) {}

// Metropolis Tests
BOOST_AUTO_TEST_CASE(stStackTests) {}
BOOST_AUTO_TEST_CASE(stStaticCall) {}
BOOST_AUTO_TEST_CASE(stReturnDataTest) {}
BOOST_AUTO_TEST_CASE(stZeroKnowledge) {}
BOOST_AUTO_TEST_CASE(stZeroKnowledge2) {}
BOOST_AUTO_TEST_CASE(stCodeCopyTest) {}
BOOST_AUTO_TEST_CASE(stBugs) {}

// Constantinople Tests
BOOST_AUTO_TEST_CASE(stShift) {}
BOOST_AUTO_TEST_CASE(stCreate2) {}
BOOST_AUTO_TEST_CASE(stExtCodeHash) {}
BOOST_AUTO_TEST_CASE(stSStoreTest) {}

// Stress Tests
BOOST_AUTO_TEST_CASE(stAttackTest) {}
BOOST_AUTO_TEST_CASE(stMemoryStressTest) {}
BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest) {}

// Bad opcodes test
BOOST_AUTO_TEST_CASE(stBadOpcode) {}

// New Tests
BOOST_AUTO_TEST_CASE(stArgsZeroOneBalance) {}
BOOST_AUTO_TEST_CASE(stTimeConsuming) {}
BOOST_AUTO_TEST_SUITE_END()  // BCGeneralStateTests Constantinople Legacy


BOOST_AUTO_TEST_SUITE(BlockchainTests)
// Tests that contain only valid blocks and check that import is correct
BOOST_FIXTURE_TEST_SUITE(ValidBlocks, LegacyConstantinoplebcValidTestFixture)
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTest) {}
BOOST_AUTO_TEST_CASE(bcExploitTest) {}
BOOST_AUTO_TEST_CASE(bcForkStressTest) {}
BOOST_AUTO_TEST_CASE(bcGasPricerTest) {}
BOOST_AUTO_TEST_CASE(bcMultiChainTest) {}
BOOST_AUTO_TEST_CASE(bcRandomBlockhashTest) {}
BOOST_AUTO_TEST_CASE(bcStateTests) {}
BOOST_AUTO_TEST_CASE(bcTotalDifficultyTest) {}
BOOST_AUTO_TEST_CASE(bcUncleSpecialTests) {}
BOOST_AUTO_TEST_CASE(bcUncleTest) {}
BOOST_AUTO_TEST_CASE(bcValidBlockTest) {}
BOOST_AUTO_TEST_CASE(bcWalletTest) {}
BOOST_AUTO_TEST_SUITE_END()

// Tests that might have invalid blocks and check that those are rejected
BOOST_FIXTURE_TEST_SUITE(InvalidBlocks, LegacyConstantinoplebcInvalidTestFixture)
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTest) {}
BOOST_AUTO_TEST_CASE(bcForgedTest) {}
BOOST_AUTO_TEST_CASE(bcInvalidHeaderTest) {}
BOOST_AUTO_TEST_CASE(bcMultiChainTest) {}
BOOST_AUTO_TEST_CASE(bcUncleHeaderValidity) {}
BOOST_AUTO_TEST_CASE(bcUncleSpecialTests) {}
BOOST_AUTO_TEST_CASE(bcUncleTest) {}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()  // BlockchainTests


BOOST_AUTO_TEST_SUITE_END()  // Constantinople
BOOST_AUTO_TEST_SUITE_END()  // LegacyTests
