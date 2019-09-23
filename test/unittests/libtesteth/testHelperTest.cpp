// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Unit tests for TestHelper functions.
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(TestHelperSuite, TestOutputHelperFixture)

BOOST_AUTO_TEST_SUITE(TranslateNetworks)
BOOST_AUTO_TEST_CASE(translateNetworks_gteIstanbul)
{
    set<string> networks = {">=Istanbul"};
    networks = test::translateNetworks(networks);
    BOOST_CHECK(contains(networks, "Istanbul"));
}

BOOST_AUTO_TEST_CASE(translateNetworks_gtConstantinople)
{
    set<string> networks = {">Constantinople"};
    networks = test::translateNetworks(networks);
    BOOST_CHECK(!contains(networks, "Constantinople"));
    BOOST_CHECK(contains(networks, "ConstantinopleFix"));
}

BOOST_AUTO_TEST_CASE(translateNetworks_gtHomestead)
{
    set<string> networks = {"Frontier", ">Homestead"};
    networks = test::translateNetworks(networks);
    BOOST_REQUIRE(networks.count("Frontier") > 0);
    BOOST_REQUIRE(networks.count("Homestead") == 0);
    for (auto const& net : test::getNetworks())
    {
        if (net != eth::Network::FrontierTest && net != eth::Network::HomesteadTest)
            BOOST_REQUIRE(networks.count(test::netIdToString(net)) > 0);
    }
}

BOOST_AUTO_TEST_CASE(translateNetworks_geHomestead)
{
    set<string> networks = {"Frontier", ">=Homestead"};
    networks = test::translateNetworks(networks);
    for (auto const& net : test::getNetworks())
        BOOST_REQUIRE(networks.count(test::netIdToString(net)) > 0);
}

BOOST_AUTO_TEST_CASE(translateNetworks_ltHomestead)
{
    set<string> networks = {"<Homestead"};
    networks = test::translateNetworks(networks);
    BOOST_REQUIRE(networks.count("Frontier") > 0);
    for (auto const& net : test::getNetworks())
    {
        if (net != eth::Network::FrontierTest)
            BOOST_REQUIRE(networks.count(test::netIdToString(net)) == 0);
    }
}

BOOST_AUTO_TEST_CASE(translateNetworks_ltTest)
{
    set<string> networks = {"<=EIP150", "<EIP158"};
    networks = test::translateNetworks(networks);
    BOOST_REQUIRE(networks.count("Frontier") > 0);
    BOOST_REQUIRE(networks.count("Homestead") > 0);
    BOOST_REQUIRE(networks.count("EIP150") > 0);
    BOOST_REQUIRE(networks.count("EIP158") == 0);
    BOOST_REQUIRE(networks.count("Byzantium") == 0);
}

BOOST_AUTO_TEST_CASE(translateNetworks_leHomestead)
{
    set<string> networks = {"<=Homestead"};
    networks = test::translateNetworks(networks);
    BOOST_REQUIRE(networks.count("Frontier") > 0);
    BOOST_REQUIRE(networks.count("Homestead") > 0);
    for (auto const& net : test::getNetworks())
    {
        if (net != eth::Network::FrontierTest && net != eth::Network::HomesteadTest)
            BOOST_REQUIRE(networks.count(test::netIdToString(net)) == 0);
    }
}

BOOST_AUTO_TEST_CASE(translateNetworks_leFrontier)
{
    set<string> networks = {"<=Frontier"};
    networks = test::translateNetworks(networks);
    BOOST_REQUIRE(networks.count("Frontier") > 0);
    for (auto const& net : test::getNetworks())
    {
        if (net != eth::Network::FrontierTest)
            BOOST_REQUIRE(networks.count(test::netIdToString(net)) == 0);
    }
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestHelper)
BOOST_AUTO_TEST_CASE(levenshteinDistance_similar)
{
    char const* word1 = "someword";
    char const* word2 = "soemword";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 2);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_similar2)
{
    char const* word1 = "sOmeWord";
    char const* word2 = "someword";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 2);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_similar3)
{
    char const* word1 = "sOmeWoRd";
    char const* word2 = "someword";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 3);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_similar4)
{
    char const* word1 = "sOmeWoRd";
    char const* word2 = "soemword";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 5);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_AgtB)
{
    char const* word1 = "someword";
    char const* word2 = "other";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 4);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_AgtB2)
{
    char const* word1 = "some long sentence here";
    char const* word2 = "other shorter phrase";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 14);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_BgtA)
{
    char const* word1 = "other";
    char const* word2 = "someword";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 4);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_BgtA2)
{
    char const* word1 = "other shorter phrase";
    char const* word2 = "some long sentence here";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 14);
}

BOOST_AUTO_TEST_CASE(levenshteinDistance_different)
{
    char const* word1 = "abdefg";
    char const* word2 = "hijklmn";
    size_t distance = test::levenshteinDistance(word1, strlen(word1), word2, strlen(word2));
    BOOST_CHECK_EQUAL(distance, 6);
}

BOOST_AUTO_TEST_CASE(getTestSuggestions)
{
    vector<string> const testList = {
        "test1", "test2", "BlockSuite", "BlockSuite/TestCase", "GeneralBlockchainTests"};
    auto list = test::testSuggestions(testList, "blocksuit");
    BOOST_CHECK(test::inArray(list, string("BlockSuite")));
}

BOOST_AUTO_TEST_CASE(getTestSuggestions2)
{
    vector<string> const testList = {"test1", "test2", "BlockSuite", "BlockSuite/TestCase",
        "GeneralBlockchainTests", "GeneralStateTests/stExample", "BCGeneralStateTests/stExample"};

    auto list = test::testSuggestions(testList, "GeneralStateTests/stExample2");
    BOOST_CHECK(test::inArray(list, string("GeneralStateTests/stExample")));
    BOOST_CHECK(test::inArray(list, string("BCGeneralStateTests/stExample")));
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
