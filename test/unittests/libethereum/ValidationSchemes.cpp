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
/** @file ValidationSchemes.cpp
 * @author Dimitry Khokhlov <dimitry@ethereum.org>
 * @date 2019
 * Validation scheme unit tests.
 */

#include <libethereum/ValidationSchemes.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>
#include <test/tools/libtesteth/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace js = json_spirit;

BOOST_FIXTURE_TEST_SUITE(ValidationSchemes, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(validateAccountObj_incomplete)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "nonce": "34"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}


BOOST_AUTO_TEST_CASE(validateAccountObj_incomplete_wrongfield)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "nonce": "34",
        "wrong": "2345"
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("Unexpected") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(validation::validateAccountObj(val.get_obj()), UnknownField, is_critical);
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_balance_wrongtype)
{
    js::mValue val;
    string s = R"({
        "balance": 1234
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("expected") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(
        validation::validateAccountObj(val.get_obj()), WrongFieldType, is_critical);
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_balance)
{
    js::mValue val;
    string s = R"({
        "balance": "1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_balancewei)
{
    js::mValue val;
    string s = R"({
        "balance": "1234",
        "wei": "1235"
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("contradicts") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(validation::validateAccountObj(val.get_obj()), UnknownField, is_critical);
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_nonce)
{
    js::mValue val;
    string s = R"({
        "nonce": "1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_code)
{
    js::mValue val;
    string s = R"({
        "code": "0x1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_codeFromFile)
{
    js::mValue val;
    string s = R"({
        "codeFromFile": "0x1234"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_just_codeFromFileAndCode)
{
    js::mValue val;
    string s = R"({
        "code": "0x1234",
        "codeFromFile": "0x1235"
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("contradicts") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(validation::validateAccountObj(val.get_obj()), UnknownField, is_critical);
}


BOOST_AUTO_TEST_CASE(validateAccountObj_just_storage)
{
    js::mValue val;
    string s = R"({
        "storage": {}
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_precompiled)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } }
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_precompiled_wei)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "wei" : "123456"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_precompiled_balance)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "balance" : "123456"
    })";
    js::read_string(s, val);
    validation::validateAccountObj(val.get_obj());
}

BOOST_AUTO_TEST_CASE(validateAccountObj_precompiled_weibalance)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "wei" : "123456",
        "balance" : "321345"
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("contradicts") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(validation::validateAccountObj(val.get_obj()), UnknownField, is_critical);
}

BOOST_AUTO_TEST_CASE(validateAccountObj_precompiled_wrongfield)
{
    js::mValue val;
    string s = R"({
        "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } },
        "transition" : "123456"
    })";
    js::read_string(s, val);
    auto is_critical = [](std::exception const& _e) {
        return string(_e.what()).find("Unexpected") != string::npos;
    };
    BOOST_CHECK_EXCEPTION(validation::validateAccountObj(val.get_obj()), UnknownField, is_critical);
}

BOOST_AUTO_TEST_SUITE_END()
