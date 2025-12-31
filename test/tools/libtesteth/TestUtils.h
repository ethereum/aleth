// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <functional>
#include <string>
#include <json/json.h>
#include <libdevcore/TransientDirectory.h>
#include <libethereum/BlockChain.h>
#include <libethereum/ClientBase.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

namespace dev
{
namespace test
{

// should be used for multithread tests
static SharedMutex x_boostTest;
#define ETH_CHECK_EQUAL(x, y) { dev::WriteGuard wg(x_boostTest); BOOST_CHECK_EQUAL(x, y); }
#define ETH_CHECK_EQUAL_COLLECTIONS(xb, xe, yb, ye) { dev::WriteGuard wg(x_boostTest); BOOST_CHECK_EQUAL_COLLECTIONS(xb, xe, yb, ye); }
#define ETH_REQUIRE(x) { dev::WriteGuard wg(x_boostTest); BOOST_REQUIRE(x); }

struct LoadTestFileFixture
{
	LoadTestFileFixture();

protected:
	Json::Value m_json;
};

struct ParallelFixture
{
	void enumerateThreads(std::function<void()> callback) const;
};

struct BlockChainFixture: public LoadTestFileFixture
{
	void enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain const&, dev::eth::State state)> callback) const;
};

struct ClientBaseFixture: public BlockChainFixture
{
	void enumerateClients(std::function<void(Json::Value const&, dev::eth::ClientBase&)> callback) const;
};

// important BOOST TEST do have problems with thread safety!!!
// BOOST_CHECK is not thread safe
// BOOST_MESSAGE is not thread safe
// http://boost.2283326.n4.nabble.com/Is-boost-test-thread-safe-td3471644.html
// http://lists.boost.org/boost-users/2010/03/57691.php
// worth reading
// https://codecrafter.wordpress.com/2012/11/01/c-unit-test-framework-adapter-part-3/
struct ParallelClientBaseFixture: public ClientBaseFixture, public ParallelFixture, public TestOutputHelperFixture
{
	void enumerateClients(std::function<void(Json::Value const&, dev::eth::ClientBase&)> callback) const;
};

struct JsonRpcFixture: public ClientBaseFixture
{
	
};

}
}
