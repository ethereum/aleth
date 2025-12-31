// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <test/tools/jsontests/StateTests.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev::test;
BOOST_FIXTURE_TEST_SUITE(StateTestTestSuite, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(singlenetOnMissingPostState, 1)
BOOST_AUTO_TEST_CASE(singlenetOnMissingPostState)
{
    std::cout << "StateTestTestSuite/singlenetOnMissingPostState test - failure is expected\n";
    std::string const s = R"(
          {
              "evmBytecode" : {
                  "_info" : {
                      "comment" : "",
                      "filling-rpc-server" : "Geth-1.9.4-unstable-96fb8391-20190913",
                      "filling-tool-version" : "retesteth-0.0.1+commit.2c0b0842.Linux.g++",
                      "lllcversion" : "Version: 0.5.12-develop.2019.9.13+commit.2d601a4f.Linux.g++",
                      "source" : "src/GeneralStateTestsFiller/stBugs/evmBytecodeFiller.json",
                      "sourceHash" : "0f222d2f60bc3607b4b2ff7ef2980ac26760634b060abb863d7cdd422e64bbee"
                  },
                  "env" : {
                      "currentCoinbase" : "0x1000000000000000000000000000000000000000",
                      "currentDifficulty" : "0x020000",
                      "currentGasLimit" : "0x54a60a4202e088",
                      "currentNumber" : "0x01",
                      "currentTimestamp" : "0x03e8",
                      "previousHash" : "0x0da7f1041f7b60aec6b67cafa0c08c10c9954d0b43737891d204ff372d166593"
                  },
                  "post" : {
                      "ConstantinopleFix" : [
                          {
                              "indexes" : {
                                  "data" : 0,
                                  "gas" : 0,
                                  "value" : 0
                              },
                              "hash" : "0x960c0b0b3f90a33ecc2bbf380769e425d01112fd92fa250c648165bf40a54a62",
                              "logs" : "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
                          }
                      ]
                  },
                  "pre" : {
                      "0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
                          "balance" : "0x38beec8feeca2598",
                          "code" : "0x",
                          "nonce" : "0x00",
                          "storage" : {
                          }
                      },
                      "0xb94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
                          "balance" : "0x00",
                          "code" : "0x67FFFFFFFFFFFFFFFF600160006000FB",
                          "nonce" : "0x3f",
                          "storage" : {
                          }
                      }
                  },
                  "transaction" : {
                      "data" : [
                          "0x"
                      ],
                      "gasLimit" : [
                          "0x01d4c0"
                      ],
                      "gasPrice" : "0x01",
                      "nonce" : "0x00",
                      "secretKey" : "0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
                      "to" : "0xb94f5374fce5edbc8e2a8697c15331677e6ebf0b",
                      "value" : [
                          "0x00"
                      ]
                  }
              }
          }
    )";

    Options const& opt = Options::get();
    Options& fakeOpt = const_cast<Options&>(opt);
    string origTestNet = fakeOpt.singleTestNet;
    fakeOpt.singleTestNet = "Byzantium";
    json_spirit::mValue input;
    json_spirit::read_string(s, input);
    StateTestSuite suite;
    suite.doTests(input, false);
    fakeOpt.singleTestNet = origTestNet;
}

BOOST_AUTO_TEST_SUITE_END()
