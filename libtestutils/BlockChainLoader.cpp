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
/** @file BlockChainLoader.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "BlockChainLoader.h"
#include "StateLoader.h"
#include "Common.h"

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::eth;

namespace dev
{
namespace test
{
dev::eth::BlockInfo toBlockInfo(Json::Value const& _json);
bytes toGenesisBlock(Json::Value const& _json);

}
}

dev::eth::BlockInfo dev::test::toBlockInfo(Json::Value const& _json)
{
	RLPStream rlpStream;
	auto size = _json.getMemberNames().size();
	rlpStream.appendList(_json["hash"].empty() ? size : (size - 1));
	rlpStream << toBytes(_json["parentHash"].asString());
	rlpStream << toBytes(_json["uncleHash"].asString());
	rlpStream << toBytes(_json["coinbase"].asString());
	rlpStream << toBytes(_json["stateRoot"].asString());
	rlpStream << toBytes(_json["transactionsTrie"].asString());
	rlpStream << toBytes(_json["receiptTrie"].asString());
	rlpStream << toBytes(_json["bloom"].asString());
	rlpStream << bigint(_json["difficulty"].asString());
	rlpStream << bigint(_json["number"].asString());
	rlpStream << bigint(_json["gasLimit"].asString());
	rlpStream << bigint(_json["gasUsed"].asString());
	rlpStream << bigint(_json["timestamp"].asString());
	rlpStream << fromHex(_json["extraData"].asString());
	rlpStream << toBytes(_json["mixHash"].asString());
	rlpStream << toBytes(_json["nonce"].asString());
	
	BlockInfo result;
	RLP rlp(rlpStream.out());
	result.populateFromHeader(rlp, IgnoreNonce);
	return result;
}

bytes dev::test::toGenesisBlock(Json::Value const& _json)
{
	BlockInfo bi = toBlockInfo(_json);
	RLPStream rlpStream;
	bi.streamRLP(rlpStream, WithNonce);
	
	RLPStream fullStream(3);
	fullStream.appendRaw(rlpStream.out());
	fullStream.appendRaw(RLPEmptyList);
	fullStream.appendRaw(RLPEmptyList);
	bi.verifyInternals(&fullStream.out());
	
	return fullStream.out();
}

BlockChainLoader::BlockChainLoader(Json::Value const& _json)
{
	// load pre state
	StateLoader sl(_json["pre"]);
	m_state = sl.state();

	// load genesisBlock
	m_bc.reset(new BlockChain(toGenesisBlock(_json["genesisBlockHeader"]), m_dir.path(), true));

	// load blocks
	for (auto const& block: _json["blocks"])
	{
		bytes rlp = toBytes(block["rlp"].asString());
		m_bc->import(rlp, m_state.db());
	}

	// sync state
	m_state.sync(*m_bc);
}
