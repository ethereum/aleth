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
/** @file ChainParams.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "ChainParams.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Log.h>
#include <libdevcore/TrieDB.h>
#include <libethcore/SealEngine.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Precompiled.h>
#include "GenesisInfo.h"
#include "State.h"
#include "Account.h"
using namespace std;
using namespace dev;
using namespace eth;
namespace js = json_spirit;


ChainParams::ChainParams()
{
	for (unsigned i = 1; i <= 4; ++i)
		genesisState[Address(i)] = Account(0, 1);
	// Setup default precompiled contracts as equal to genesis of Frontier.
	precompiled.insert(make_pair(Address(1), PrecompiledContract(3000, 0, PrecompiledRegistrar::executor("ecrecover"))));
	precompiled.insert(make_pair(Address(2), PrecompiledContract(60, 12, PrecompiledRegistrar::executor("sha256"))));
	precompiled.insert(make_pair(Address(3), PrecompiledContract(600, 120, PrecompiledRegistrar::executor("ripemd160"))));
	precompiled.insert(make_pair(Address(4), PrecompiledContract(15, 3, PrecompiledRegistrar::executor("identity"))));
}

ChainParams::ChainParams(string const& _json, h256 const& _stateRoot)
{
	*this = loadConfig(_json, _stateRoot);
}

ChainParams ChainParams::loadConfig(string const& _json, h256 const& _stateRoot) const
{
	ChainParams cp(*this);
	js::mValue val;
	json_spirit::read_string(_json, val);
	js::mObject obj = val.get_obj();

	cp.sealEngineName = obj["sealEngine"].get_str();
	// params
	js::mObject params = obj["params"].get_obj();
	cp.accountStartNonce = u256(fromBigEndian<u256>(fromHex(params["accountStartNonce"].get_str())));
	cp.maximumExtraDataSize = u256(fromBigEndian<u256>(fromHex(params["maximumExtraDataSize"].get_str())));
	cp.tieBreakingGas = params.count("tieBreakingGas") ? params["tieBreakingGas"].get_bool() : true;
	cp.blockReward = u256(fromBigEndian<u256>(fromHex(params["blockReward"].get_str())));
	for (auto i: params)
		if (i.first != "accountStartNonce" && i.first != "maximumExtraDataSize" && i.first != "blockReward" && i.first != "tieBreakingGas")
			cp.otherParams[i.first] = i.second.get_str();
	// genesis
	string genesisStr = json_spirit::write_string(obj["genesis"], false);
	cp = cp.loadGenesis(genesisStr, _stateRoot);
	// genesis state
	string genesisStateStr = json_spirit::write_string(obj["accounts"], false);
	cp = cp.loadGenesisState(genesisStateStr, _stateRoot);
	return cp;
}

ChainParams ChainParams::loadGenesisState(string const& _json, h256 const& _stateRoot) const
{
	ChainParams cp(*this);
	cp.genesisState = jsonToAccountMap(_json, cp.accountStartNonce, nullptr, &cp.precompiled);
	cp.stateRoot = _stateRoot ? _stateRoot : cp.calculateStateRoot(true);
	return cp;
}

ChainParams ChainParams::loadGenesis(string const& _json, h256 const& _stateRoot) const
{
	ChainParams cp(*this);

	js::mValue val;
	json_spirit::read_string(_json, val);
	js::mObject genesis = val.get_obj();

	cp.parentHash = h256(genesis["parentHash"].get_str());
	cp.author = genesis.count("coinbase") ? h160(genesis["coinbase"].get_str()) : h160(genesis["author"].get_str());
	cp.difficulty = genesis.count("difficulty") ? u256(fromBigEndian<u256>(fromHex(genesis["difficulty"].get_str()))) : 0;
	cp.gasLimit = u256(fromBigEndian<u256>(fromHex(genesis["gasLimit"].get_str())));
	cp.gasUsed = genesis.count("gasUsed") ? u256(fromBigEndian<u256>(fromHex(genesis["gasUsed"].get_str()))) : 0;
	cp.timestamp = u256(fromBigEndian<u256>(fromHex(genesis["timestamp"].get_str())));
	cp.extraData = bytes(fromHex(genesis["extraData"].get_str()));

	// magic code for handling ethash stuff:
	if ((genesis.count("mixhash") || genesis.count("mixHash")) && genesis.count("nonce"))
	{
		h256 mixHash(genesis[genesis.count("mixhash") ? "mixhash" : "mixHash"].get_str());
		h64 nonce(genesis["nonce"].get_str());
		cp.sealFields = 2;
		cp.sealRLP = rlp(mixHash) + rlp(nonce);
	}
	cp.stateRoot = _stateRoot ? _stateRoot : cp.calculateStateRoot();
	return cp;
}

SealEngineFace* ChainParams::createSealEngine()
{
	SealEngineFace* ret = SealEngineRegistrar::create(sealEngineName);
	assert(ret && "Seal engine not found");
	if (!ret)
		return nullptr;
	ret->setChainParams(*this);
	if (sealRLP.empty())
	{
		sealFields = ret->sealFields();
		sealRLP = ret->sealRLP();
	}
	return ret;
}

void ChainParams::populateFromGenesis(bytes const& _genesisRLP, AccountMap const& _state)
{
	BlockHeader bi(_genesisRLP, RLP(&_genesisRLP)[0].isList() ? BlockData : HeaderData);
	parentHash = bi.parentHash();
	author = bi.author();
	difficulty = bi.difficulty();
	gasLimit = bi.gasLimit();
	gasUsed = bi.gasUsed();
	timestamp = bi.timestamp();
	extraData = bi.extraData();
	genesisState = _state;
	RLP r(_genesisRLP);
	sealFields = r[0].itemCount() - BlockHeader::BasicFields;
	sealRLP.clear();
	for (unsigned i = BlockHeader::BasicFields; i < r[0].itemCount(); ++i)
		sealRLP += r[0][i].data();

	calculateStateRoot(true);

	auto b = genesisBlock();
	if (b != _genesisRLP)
	{
		cdebug << "Block passed:" << bi.hash() << bi.hash(WithoutSeal);
		cdebug << "Genesis now:" << BlockHeader::headerHashFromBlock(b);
		cdebug << RLP(b);
		cdebug << RLP(_genesisRLP);
		throw 0;
	}
}

h256 ChainParams::calculateStateRoot(bool _force) const
{
	MemoryDB db;
	SecureTrieDB<Address, MemoryDB> state(&db);
	state.init();
	if (!stateRoot || _force)
	{
		// TODO: use hash256
		//stateRoot = hash256(toBytesMap(gs));
		dev::eth::commit(genesisState, state);
		stateRoot = state.root();
	}
	return stateRoot;
}

bytes ChainParams::genesisBlock() const
{
	RLPStream block(3);

	calculateStateRoot();

	block.appendList(BlockHeader::BasicFields + sealFields)
			<< parentHash
			<< EmptyListSHA3	// sha3(uncles)
			<< author
			<< stateRoot
			<< EmptyTrie		// transactions
			<< EmptyTrie		// receipts
			<< LogBloom()
			<< difficulty
			<< 0				// number
			<< gasLimit
			<< gasUsed			// gasUsed
			<< timestamp
			<< extraData;
	block.appendRaw(sealRLP, sealFields);
	block.appendRaw(RLPEmptyList);
	block.appendRaw(RLPEmptyList);
	return block.out();
}
