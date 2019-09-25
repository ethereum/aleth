// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Transaaction test functions.
#include <test/tools/libtesteth/BlockChainHelper.h>
#include "test/tools/libtesteth/TestHelper.h"
#include <libethcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libevm/VMFace.h>
using namespace dev;
using namespace eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(libethereum, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(TransactionGasRequired)
{
    // Transaction data is 0358ac39584bc98a7c979f984b03, 14 bytes
    Transaction tr(fromHex("0xf86d800182521c94095e7baea6a6c7c4c2dfeb977efac326af552d870a8e0358ac39584bc98a7c979f984b031ba048b55bfa915ac795c431978d8a6a992b628d557da5ff759b307d495a36649353a0efffd310ac743f371de3b9f7f9cb56c0b28ad43601b4ab949f53faa07bd2c804"), CheckTransaction::None);
    BOOST_CHECK_EQUAL(tr.baseGasRequired(FrontierSchedule), 14 * 68 + 21000);
    BOOST_CHECK_EQUAL(tr.baseGasRequired(IstanbulSchedule), 14 * 16 + 21000);
}

BOOST_AUTO_TEST_CASE(TransactionWithEmptyRecipient)
{
    // recipient RLP is 0x80 (empty array)
    auto txRlp = fromHex(
        "0xf84c8014830493e080808026a02f23977c68f851bbec8619510a4acdd34805270d97f5714b003efe7274914c"
        "a2a05874022b26e0d88807bdcc59438f86f5a82e24afefad5b6a67ae853896fe2b37");
    Transaction tx(txRlp, CheckTransaction::None);  // shouldn't throw

    // recipient RLP is 0xc0 (empty list)
    txRlp = fromHex(
        "0xf84c8014830493e0c0808026a02f23977c68f851bbec8619510a4acdd34805270d97f5714b003efe7274914c"
        "a2a05874022b26e0d88807bdcc59438f86f5a82e24afefad5b6a67ae853896fe2b37");
    BOOST_REQUIRE_THROW(Transaction(txRlp, CheckTransaction::None), InvalidTransactionFormat);
}

BOOST_AUTO_TEST_CASE(TransactionNotReplayProtected)
{
    auto txRlp = fromHex(
        "0xf86d800182521c94095e7baea6a6c7c4c2dfeb977efac326af552d870a8e0358ac39584bc98a7c979f984b03"
        "1ba048b55bfa915ac795c431978d8a6a992b628d557da5ff759b307d495a36649353a0efffd310ac743f371de3"
        "b9f7f9cb56c0b28ad43601b4ab949f53faa07bd2c804");
    Transaction tx(txRlp, CheckTransaction::None);
    tx.checkChainId(1234);  // any chain ID is accepted for not replay protected tx

    RLPStream txRlpStream;
    tx.streamRLP(txRlpStream);
    BOOST_REQUIRE(txRlpStream.out() == txRlp);
}

BOOST_AUTO_TEST_CASE(TransactionChainIDMax64Bit)
{
    // recoveryID = 0, v = 36893488147419103265
    auto txRlp1 = fromHex(
        "0xf86e808698852840a46f82d6d894095e7baea6a6c7c4c2dfeb977efac326af552d8780808902000000000000"
        "0021a098ff921201554726367d2be8c804a7ff89ccf285ebc57dff8ae4c44b9c19ac4aa01887321be575c8095f"
        "789dd4c743dfe42c1820f9231f98a962b210e3ac2452a3");
    Transaction tx1{txRlp1, CheckTransaction::None};
    tx1.checkChainId(std::numeric_limits<uint64_t>::max());

    // recoveryID = 1, v = 36893488147419103266
    auto txRlp2 = fromHex(
        "0xf86e808698852840a46f82d6d894095e7baea6a6c7c4c2dfeb977efac326af552d8780808902000000000000"
        "0022a098ff921201554726367d2be8c804a7ff89ccf285ebc57dff8ae4c44b9c19ac4aa01887321be575c8095f"
        "789dd4c743dfe42c1820f9231f98a962b210e3ac2452a3");
    Transaction tx2{txRlp2, CheckTransaction::None};
    tx2.checkChainId(std::numeric_limits<uint64_t>::max());
}

BOOST_AUTO_TEST_CASE(TransactionChainIDBiggerThan64Bit)
{
    // recoveryID = 0, v = 184467440737095516439
    auto txRlp1 = fromHex(
        "0xf86a03018255f094b94f5374fce5edbc8e2a8697c15331677e6ebf0b0a825544890a0000000000000117a098"
        "ff921201554726367d2be8c804a7ff89ccf285ebc57dff8ae4c44b9c19ac4aa08887321be575c8095f789dd4c7"
        "43dfe42c1820f9231f98a962b210e3ac2452a3");
    BOOST_REQUIRE_THROW(Transaction(txRlp1, CheckTransaction::None), InvalidSignature);

    // recoveryID = 1, v = 184467440737095516440
    auto txRlp2 = fromHex(
        "0xf86a03018255f094b94f5374fce5edbc8e2a8697c15331677e6ebf0b0a825544890a0000000000000118a098"
        "ff921201554726367d2be8c804a7ff89ccf285ebc57dff8ae4c44b9c19ac4aa08887321be575c8095f789dd4c7"
        "43dfe42c1820f9231f98a962b210e3ac2452a3");
    BOOST_REQUIRE_THROW(Transaction(txRlp2, CheckTransaction::None), InvalidSignature);
}

BOOST_AUTO_TEST_CASE(TransactionReplayProtected)
{
    auto txRlp = fromHex(
        "0xf86c098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a76400008025"
        "a028ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276a067cbe9d8997f761aecb703"
        "304b3800ccf555c9f3dc64214b297fb1966a3b6d83");
    Transaction tx(txRlp, CheckTransaction::None);
    tx.checkChainId(1);
    BOOST_REQUIRE_THROW(tx.checkChainId(123), InvalidSignature);

    RLPStream txRlpStream;
    tx.streamRLP(txRlpStream);
    BOOST_REQUIRE(txRlpStream.out() == txRlp);
}

BOOST_AUTO_TEST_CASE(ExecutionResultOutput)
{
    std::stringstream buffer;
    ExecutionResult exRes;

    exRes.gasUsed = u256("12345");
    exRes.newAddress = Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    exRes.output = fromHex("001122334455");

    buffer << exRes;
    BOOST_CHECK_MESSAGE(buffer.str() == "{12345, a94f5374fce5edbc8e2a8697c15331677e6ebf0b, 001122334455}", "Error ExecutionResultOutput");
}

BOOST_AUTO_TEST_CASE(transactionExceptionOutput)
{
    std::stringstream buffer;
    buffer << TransactionException::BadInstruction;
    BOOST_CHECK_MESSAGE(buffer.str() == "BadInstruction", "Error output TransactionException::BadInstruction");
    buffer.str(std::string());

    buffer << TransactionException::None;
    BOOST_CHECK_MESSAGE(buffer.str() == "None", "Error output TransactionException::None");
    buffer.str(std::string());

    buffer << TransactionException::BadRLP;
    BOOST_CHECK_MESSAGE(buffer.str() == "BadRLP", "Error output TransactionException::BadRLP");
    buffer.str(std::string());

    buffer << TransactionException::InvalidFormat;
    BOOST_CHECK_MESSAGE(buffer.str() == "InvalidFormat", "Error output TransactionException::InvalidFormat");
    buffer.str(std::string());

    buffer << TransactionException::OutOfGasIntrinsic;
    BOOST_CHECK_MESSAGE(buffer.str() == "OutOfGasIntrinsic", "Error output TransactionException::OutOfGasIntrinsic");
    buffer.str(std::string());

    buffer << TransactionException::InvalidSignature;
    BOOST_CHECK_MESSAGE(buffer.str() == "InvalidSignature", "Error output TransactionException::InvalidSignature");
    buffer.str(std::string());

    buffer << TransactionException::InvalidNonce;
    BOOST_CHECK_MESSAGE(buffer.str() == "InvalidNonce", "Error output TransactionException::InvalidNonce");
    buffer.str(std::string());

    buffer << TransactionException::NotEnoughCash;
    BOOST_CHECK_MESSAGE(buffer.str() == "NotEnoughCash", "Error output TransactionException::NotEnoughCash");
    buffer.str(std::string());

    buffer << TransactionException::OutOfGasBase;
    BOOST_CHECK_MESSAGE(buffer.str() == "OutOfGasBase", "Error output TransactionException::OutOfGasBase");
    buffer.str(std::string());

    buffer << TransactionException::BlockGasLimitReached;
    BOOST_CHECK_MESSAGE(buffer.str() == "BlockGasLimitReached", "Error output TransactionException::BlockGasLimitReached");
    buffer.str(std::string());

    buffer << TransactionException::BadInstruction;
    BOOST_CHECK_MESSAGE(buffer.str() == "BadInstruction", "Error output TransactionException::BadInstruction");
    buffer.str(std::string());

    buffer << TransactionException::BadJumpDestination;
    BOOST_CHECK_MESSAGE(buffer.str() == "BadJumpDestination", "Error output TransactionException::BadJumpDestination");
    buffer.str(std::string());

    buffer << TransactionException::OutOfGas;
    BOOST_CHECK_MESSAGE(buffer.str() == "OutOfGas", "Error output TransactionException::OutOfGas");
    buffer.str(std::string());

    buffer << TransactionException::OutOfStack;
    BOOST_CHECK_MESSAGE(buffer.str() == "OutOfStack", "Error output TransactionException::OutOfStack");
    buffer.str(std::string());

    buffer << TransactionException::StackUnderflow;
    BOOST_CHECK_MESSAGE(buffer.str() == "StackUnderflow", "Error output TransactionException::StackUnderflow");
    buffer.str(std::string());

    buffer << TransactionException(-1);
    BOOST_CHECK_MESSAGE(buffer.str() == "Unknown", "Error output TransactionException::StackUnderflow");
    buffer.str(std::string());
}

BOOST_AUTO_TEST_CASE(toTransactionExceptionConvert)
{
    RLPException rlpEx;  // toTransactionException(*(dynamic_cast<Exception*>
    BOOST_CHECK_MESSAGE(toTransactionException(rlpEx) == TransactionException::BadRLP,
        "RLPException !=> TransactionException");
    OutOfGasIntrinsic oogEx;
    BOOST_CHECK_MESSAGE(toTransactionException(oogEx) == TransactionException::OutOfGasIntrinsic, "OutOfGasIntrinsic !=> TransactionException");
    InvalidSignature sigEx;
    BOOST_CHECK_MESSAGE(toTransactionException(sigEx) == TransactionException::InvalidSignature, "InvalidSignature !=> TransactionException");
    OutOfGasBase oogbEx;
    BOOST_CHECK_MESSAGE(toTransactionException(oogbEx) == TransactionException::OutOfGasBase, "OutOfGasBase !=> TransactionException");
    InvalidNonce nonceEx;
    BOOST_CHECK_MESSAGE(toTransactionException(nonceEx) == TransactionException::InvalidNonce, "InvalidNonce !=> TransactionException");
    NotEnoughCash cashEx;
    BOOST_CHECK_MESSAGE(toTransactionException(cashEx) == TransactionException::NotEnoughCash, "NotEnoughCash !=> TransactionException");
    BlockGasLimitReached blGasEx;
    BOOST_CHECK_MESSAGE(toTransactionException(blGasEx) == TransactionException::BlockGasLimitReached, "BlockGasLimitReached !=> TransactionException");
    BadInstruction badInsEx;
    BOOST_CHECK_MESSAGE(toTransactionException(badInsEx) == TransactionException::BadInstruction, "BadInstruction !=> TransactionException");
    BadJumpDestination badJumpEx;
    BOOST_CHECK_MESSAGE(toTransactionException(badJumpEx) == TransactionException::BadJumpDestination, "BadJumpDestination !=> TransactionException");
    OutOfGas oogEx2;
    BOOST_CHECK_MESSAGE(toTransactionException(oogEx2) == TransactionException::OutOfGas, "OutOfGas !=> TransactionException");
    OutOfStack oosEx;
    BOOST_CHECK_MESSAGE(toTransactionException(oosEx) == TransactionException::OutOfStack, "OutOfStack !=> TransactionException");
    StackUnderflow stackEx;
    BOOST_CHECK_MESSAGE(toTransactionException(stackEx) == TransactionException::StackUnderflow, "StackUnderflow !=> TransactionException");
    Exception notEx;
    BOOST_CHECK_MESSAGE(toTransactionException(notEx) == TransactionException::Unknown, "Unexpected should be TransactionException::Unknown");
}

BOOST_AUTO_TEST_CASE(GettingSenderForUnsignedTransactionThrows)
{
    Transaction tx(0, 0, 10000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0);
    BOOST_CHECK(!tx.hasSignature());

    BOOST_REQUIRE_THROW(tx.sender(), TransactionIsUnsigned);
}

BOOST_AUTO_TEST_CASE(GettingSignatureForUnsignedTransactionThrows)
{
    Transaction tx(0, 0, 10000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0);
    BOOST_REQUIRE_THROW(tx.signature(), TransactionIsUnsigned);
}

BOOST_AUTO_TEST_CASE(StreamRLPWithSignatureForUnsignedTransactionThrows)
{
    Transaction tx(0, 0, 10000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0);
    RLPStream s;
    BOOST_REQUIRE_THROW(tx.streamRLP(s, IncludeSignature::WithSignature, false), TransactionIsUnsigned);
}

BOOST_AUTO_TEST_CASE(CheckLowSForUnsignedTransactionThrows)
{
    Transaction tx(0, 0, 10000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0);
    BOOST_REQUIRE_THROW(tx.checkLowS(), TransactionIsUnsigned);
}

BOOST_AUTO_TEST_SUITE_END()
