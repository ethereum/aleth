// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/**
 * Validation scheme unit tests.
 */

#include <gtest/gtest.h>
#include <libdevcore/Exceptions.h>
#include <libethereum/ValidationSchemes.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
namespace js = json_spirit;

TEST(validation, validateAccountObj_incomplete)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "nonce": "34"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_incomplete_wrongfield)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "nonce": "34",
        "wrong": "2345"
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), UnknownField);
}

TEST(validation, validateAccountObj_just_balance_wrongtype)
{
    js::mValue val;
    string s = R"({
        "balance": 1234
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), WrongFieldType);
}

TEST(validation, validateAccountObj_just_balance)
{
    js::mValue val;
    string s = R"({
        "balance": "1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_just_balancewei)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "wei": "1235"
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), UnknownField);
}

TEST(validation, validateAccountObj_just_nonce)
{
    js::mValue val;
    string s = R"({
        "nonce": "1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_just_code)
{
    js::mValue val;
    string s = R"({
        "code": "0x1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_just_codeFromFile)
{
    js::mValue val;
    string s = R"({
        "codeFromFile": "0x1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_just_codeFromFileAndCode)
{
    js::mValue val;
    string s = R"({
        "code": "0x1234",
        "codeFromFile": "0x1235"
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), UnknownField);
}


TEST(validation, validateAccountObj_just_storage)
{
    js::mValue val;
    string s = R"({
        "storage": {}
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_precompiled)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } }
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_precompiled_wei)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "wei" : "123456"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_precompiled_balance)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "balance" : "123456"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

TEST(validation, validateAccountObj_precompiled_weibalance)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "wei" : "123456",
        "balance" : "321345"
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), UnknownField);
}

TEST(validation, validateAccountObj_precompiled_wrongfield)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "transition" : "123456"
    })";
    js::read_string(s, val);
    EXPECT_THROW(validation::validateAccountObj(val.get_obj()), UnknownField);
}

