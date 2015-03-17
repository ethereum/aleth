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
/** @file TestUtils.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once

#include <functional>
#include <string>
#include <boost/filesystem.hpp>
#include <json/json.h>
#include <libethereum/BlockChain.h>
#include <libethereum/InterfaceStub.h>

namespace dev
{
namespace test
{

bytes toBytes(std::string const& _str);


class ShortLivingDirectory
{
public:
	ShortLivingDirectory(std::string const& _path) : m_path(_path) { boost::filesystem::create_directories(m_path); }
	~ShortLivingDirectory() { boost::filesystem::remove_all(m_path); }
	
private:
	std::string m_path;
};

class TestUtils
{
public:
	static dev::eth::State toState(Json::Value const& _json);
};

struct LoadTestFileFixture
{
	LoadTestFileFixture();
	
	static bool m_loaded;
	static Json::Value m_json;
};

struct BlockChainFixture: public LoadTestFileFixture
{
	void enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain&, dev::eth::State state)> callback);
};

struct InterfaceStubFixture: public BlockChainFixture
{
	void enumerateInterfaces(std::function<void(Json::Value const&, dev::eth::InterfaceStub&)> callback);
};

struct JsonRpcFixture: public InterfaceStubFixture
{
	
};

}
}
