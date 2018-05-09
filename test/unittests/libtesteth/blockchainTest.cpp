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
/** @file blockchainTest.cpp
 * Unit tests for blockchain test filling/execution.
 */

#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/jsontests/BlockChainTests.h>

#include <boost/test/unit_test.hpp>

using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(BlockChainTestSuite, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(fillingExpectationOnMultipleNetworks)
{
    std::string const s = R"(
        {
            "fillingExpectationOnMultipleNetworks" : {
                "blocks" : [
                ],
                "expect" : [
                    {
                        "network" : ["EIP150", "EIP158"],
                        "result" : {
                            "0x1000000000000000000000000000000000000000" : {
                                "balance" : "0x00"
                            },
                            "0x1000000000000000000000000000000000000001" : {
                                "balance" : "0x00"
                            }
                        }
                    }
                ],
                "genesisBlockHeader" : {
                    "bloom" : "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                    "coinbase" : "2adc25665018aa1fe0e6bc666dac8fc2697ff9ba",
                    "difficulty" : "131072",
                    "extraData" : "0x42",
                    "gasLimit" : "0x7fffffffffffffff",
                    "gasUsed" : "0",
                    "mixHash" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "nonce" : "0x0102030405060708",
                    "number" : "0",
                    "parentHash" : "0x0000000000000000000000000000000000000000000000000000000000000000",
                    "receiptTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "stateRoot" : "0xf99eb1626cfa6db435c0836235942d7ccaa935f1ae247d3f1c21e495685f903a",
                    "timestamp" : "0x03b6",
                    "transactionsTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "uncleHash" : "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
                },
                "pre" : {
                    "0x1000000000000000000000000000000000000000" : {
                        "balance" : "0x00",
                        "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                        "nonce" : "0x00",
                        "storage" : {
                        }
                    },
                "0x1000000000000000000000000000000000000001" : {
                    "balance" : "0x00",
                    "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                    "nonce" : "0x00",
                    "storage" : {
                    }
                }
        }
    )";
    json_spirit::mValue input;
    json_spirit::read_string(s, input);
    BlockchainTestSuite suite;
    json_spirit::mValue output = suite.doTests(input, true);
    BOOST_CHECK_MESSAGE(output.get_obj().size() == 2, "A wrong number of tests were generated.");
}

BOOST_AUTO_TEST_CASE(fillingExpectationOnSingleNetwork)
{
    std::string const s = R"(
        {
            "fillingExpectationOnSingleNetwork" : {
                "blocks" : [
                ],
                "expect" : [
                    {
                        "network" : "EIP150",
                        "result" : {
                            "0x1000000000000000000000000000000000000000" : {
                                "balance" : "0x00"
                            },
                            "0x1000000000000000000000000000000000000001" : {
                                "balance" : "0x00"
                            }
                        }
                    }
                ],
                "genesisBlockHeader" : {
                    "bloom" : "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                    "coinbase" : "2adc25665018aa1fe0e6bc666dac8fc2697ff9ba",
                    "difficulty" : "131072",
                    "extraData" : "0x42",
                    "gasLimit" : "0x7fffffffffffffff",
                    "gasUsed" : "0",
                    "mixHash" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "nonce" : "0x0102030405060708",
                    "number" : "0",
                    "parentHash" : "0x0000000000000000000000000000000000000000000000000000000000000000",
                    "receiptTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "stateRoot" : "0xf99eb1626cfa6db435c0836235942d7ccaa935f1ae247d3f1c21e495685f903a",
                    "timestamp" : "0x03b6",
                    "transactionsTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "uncleHash" : "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
                },
                "pre" : {
                    "0x1000000000000000000000000000000000000000" : {
                        "balance" : "0x00",
                        "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                        "nonce" : "0x00",
                        "storage" : {
                        }
                    },
                "0x1000000000000000000000000000000000000001" : {
                    "balance" : "0x00",
                    "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                    "nonce" : "0x00",
                    "storage" : {
                    }
                }
        }
    )";
    json_spirit::mValue input;
    json_spirit::read_string(s, input);
    BlockchainTestSuite suite;
    json_spirit::mValue output = suite.doTests(input, true);
    BOOST_CHECK_MESSAGE(output.get_obj().size() == 1, "A wrong number of tests were generated.");
}


BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(fillingWithWrongExpectation, 2)
BOOST_AUTO_TEST_CASE(fillingWithWrongExpectation)
{
    cout << "BlockChainTestSuite/fillingWithWrongExpectation test - failure is expected\n";

    std::string const s = R"(
        {
            "fillingWithWrongExpectation" : {
                "blocks" : [
                ],
                "expect" : [
                    {
                        "network" : ["EIP150"],
                        "result" : {
                            "0x1000000000000000000000000000000000000000" : {
                                "balance" : "0x01"
                            },
                            "0x1000000000000000000000000000000000000001" : {
                                "balance" : "0x00"
                            }
                        }
                    }
                ],
                "genesisBlockHeader" : {
                    "bloom" : "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                    "coinbase" : "2adc25665018aa1fe0e6bc666dac8fc2697ff9ba",
                    "difficulty" : "131072",
                    "extraData" : "0x42",
                    "gasLimit" : "0x7fffffffffffffff",
                    "gasUsed" : "0",
                    "mixHash" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "nonce" : "0x0102030405060708",
                    "number" : "0",
                    "parentHash" : "0x0000000000000000000000000000000000000000000000000000000000000000",
                    "receiptTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "stateRoot" : "0xf99eb1626cfa6db435c0836235942d7ccaa935f1ae247d3f1c21e495685f903a",
                    "timestamp" : "0x03b6",
                    "transactionsTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
                    "uncleHash" : "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
                },
                "pre" : {
                    "0x1000000000000000000000000000000000000000" : {
                        "balance" : "0x00",
                        "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                        "nonce" : "0x00",
                        "storage" : {
                        }
                    },
                "0x1000000000000000000000000000000000000001" : {
                    "balance" : "0x00",
                    "code" : "0x60004060005563ffffffff4060015567ffffffffffffffff406002556fffffffffffffffffffffffffffffffff406003557fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40600455",
                    "nonce" : "0x00",
                    "storage" : {
                    }
                }
        }
    )";
    json_spirit::mValue input;
    json_spirit::read_string(s, input);

    BlockchainTestSuite suite;
    json_spirit::mValue output = suite.doTests(input, true);
    BOOST_CHECK_MESSAGE(output.get_obj().size() == 1, "A wrong number of tests were generated.");
}

BOOST_AUTO_TEST_SUITE_END()
