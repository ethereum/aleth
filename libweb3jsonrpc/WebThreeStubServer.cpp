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
/** @file WebThreeStubServer.cpp
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#include "WebThreeStubServer.h"
#include <libevmface/Instruction.h>
#include <liblll/Compiler.h>
#include <libethereum/Client.h>
#include <libwebthree/WebThree.h>
#include <libdevcore/CommonJS.h>
#include <boost/filesystem.hpp>
#include <libdevcrypto/FileSystem.h>
#include <libwhisper/Message.h>
#include <libwhisper/WhisperHost.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

static Json::Value toJson(dev::eth::BlockInfo const& _bi)
{
	Json::Value res;
	res["hash"] = boost::lexical_cast<string>(_bi.hash);
	res["parentHash"] = toJS(_bi.parentHash);
	res["sha3Uncles"] = toJS(_bi.sha3Uncles);
	res["miner"] = toJS(_bi.coinbaseAddress);
	res["stateRoot"] = toJS(_bi.stateRoot);
	res["transactionsRoot"] = toJS(_bi.transactionsRoot);
	res["difficulty"] = toJS(_bi.difficulty);
	res["number"] = (int)_bi.number;
	res["minGasPrice"] = toJS(_bi.minGasPrice);
	res["gasLimit"] = (int)_bi.gasLimit;
	res["timestamp"] = (int)_bi.timestamp;
	res["extraData"] = jsFromBinary(_bi.extraData);
	res["nonce"] = toJS(_bi.nonce);
	return res;
}

static Json::Value toJson(dev::eth::PastMessage const& _t)
{
	Json::Value res;
	res["input"] = jsFromBinary(_t.input);
	res["output"] = jsFromBinary(_t.output);
	res["to"] = toJS(_t.to);
	res["from"] = toJS(_t.from);
	res["value"] = jsToDecimal(toJS(_t.value));
	res["origin"] = toJS(_t.origin);
	res["timestamp"] = toJS(_t.timestamp);
	res["coinbase"] = toJS(_t.coinbase);
	res["block"] =  toJS(_t.block);
	Json::Value path;
	for (int i: _t.path)
		path.append(i);
	res["path"] = path;
	res["number"] = (int)_t.number;
	return res;
}

static Json::Value toJson(dev::eth::PastMessages const& _pms)
{
	Json::Value res;
	for (dev::eth::PastMessage const& t: _pms)
		res.append(toJson(t));
	return res;
}

static Json::Value toJson(dev::eth::Transaction const& _t)
{
	Json::Value res;
	res["hash"] = toJS(_t.sha3());
	res["input"] = jsFromBinary(_t.data);
	res["to"] = toJS(_t.receiveAddress);
	res["from"] = toJS(_t.sender());
	res["gas"] = (int)_t.gas;
	res["gasPrice"] = toJS(_t.gasPrice);
	res["nonce"] = toJS(_t.nonce);
	res["value"] = toJS(_t.value);
	return res;
}

static dev::eth::MessageFilter toMessageFilter(Json::Value const& _json)
{
	dev::eth::MessageFilter filter;
	if (!_json.isObject() || _json.empty())
		return filter;

	if (!_json["earliest"].empty())
		filter.withEarliest(_json["earliest"].asInt());
	if (!_json["latest"].empty())
		filter.withLatest(_json["lastest"].asInt());
	if (!_json["max"].empty())
		filter.withMax(_json["max"].asInt());
	if (!_json["skip"].empty())
		filter.withSkip(_json["skip"].asInt());
	if (!_json["from"].empty())
	{
		if (_json["from"].isArray())
			for (auto i : _json["from"])
				filter.from(jsToAddress(i.asString()));
		else
			filter.from(jsToAddress(_json["from"].asString()));
	}
	if (!_json["to"].empty())
	{
		if (_json["to"].isArray())
			for (auto i : _json["to"])
				filter.from(jsToAddress(i.asString()));
		else
			filter.from(jsToAddress(_json["to"].asString()));
	}
	if (!_json["altered"].empty())
	{
		if (_json["altered"].isArray())
			for (auto i: _json["altered"])
				if (i.isObject())
					filter.altered(jsToAddress(i["id"].asString()), jsToU256(i["at"].asString()));
				else
					filter.altered((jsToAddress(i.asString())));
				else if (_json["altered"].isObject())
					filter.altered(jsToAddress(_json["altered"]["id"].asString()), jsToU256(_json["altered"]["at"].asString()));
				else
					filter.altered(jsToAddress(_json["altered"].asString()));
	}
	return filter;
}

static shh::Message toMessage(Json::Value const& _json)
{
	shh::Message ret;
	if (!_json["from"].empty())
		ret.setFrom(jsToPublic(_json["from"].asString()));
	if (!_json["to"].empty())
		ret.setTo(jsToPublic(_json["to"].asString()));
	if (!_json["payload"].empty())
		ret.setPayload(asBytes(_json["payload"].asString()));
	return ret;
}

static shh::Envelope toSealed(Json::Value const& _json, shh::Message const& _m, Secret _from)
{
	unsigned ttl = 50;
	unsigned workToProve = 50;
	shh::BuildTopic bt;

	if (!_json["ttl"].empty())
		ttl = _json["ttl"].asInt();
	if (!_json["workToProve"].empty())
		workToProve = _json["workToProve"].asInt();
	if (!_json["topic"].empty())
	{
		if (_json["topic"].isString())
			bt.shift(asBytes(jsPadded(_json["topic"].asString(), 32)));
		else if (_json["topic"].isArray())
			for (auto i: _json["topic"])
				bt.shift(asBytes(jsPadded(i.asString(), 32)));
	}
	return _m.seal(_from, bt, ttl, workToProve);
}

static pair<shh::TopicMask, Public> toWatch(Json::Value const& _json)
{
	shh::BuildTopicMask bt(shh::BuildTopicMask::Empty);
	Public to;

	if (!_json["to"].empty())
		to = jsToPublic(_json["to"].asString());

	if (!_json["topic"].empty())
	{
		if (_json["topic"].isString())
			bt.shift(asBytes(jsPadded(_json["topic"].asString(), 32)));
		else if (_json["topic"].isArray())
			for (auto i: _json["topic"])
			{
				if (i.isString())
					bt.shift(asBytes(jsPadded(i.asString(), 32)));
				else
					bt.shift();
			}
	}
	return make_pair(bt.toTopicMask(), to);
}

static Json::Value toJson(h256 const& _h, shh::Envelope const& _e, shh::Message const& _m)
{
	Json::Value res;
	res["hash"] = toJS(_h);
	res["expiry"] = (int)_e.expiry();
	res["sent"] = (int)_e.sent();
	res["ttl"] = (int)_e.ttl();
	res["workProved"] = (int)_e.workProved();
	res["topic"] = toJS(_e.topic());
	res["payload"] = asString(_m.payload());
	res["from"] = toJS(_m.from());
	res["to"] = toJS(_m.to());
	return res;
}


WebThreeStubServer::WebThreeStubServer(jsonrpc::AbstractServerConnector* _conn, WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts):
	AbstractWebThreeStubServer(_conn),
	m_web3(_web3)
{
	setAccounts(_accounts);
	auto path = getDataDir() + "/.web3";
	boost::filesystem::create_directories(path);
	ldb::Options o;
	o.create_if_missing = true;
	ldb::DB::Open(o, path, &m_db);
}

void WebThreeStubServer::setAccounts(std::vector<dev::KeyPair> const& _accounts)
{
	m_accounts.clear();
	for (auto i: _accounts)
		m_accounts[i.address()] = i.secret();
}

void WebThreeStubServer::setIdentities(std::vector<dev::KeyPair> const& _ids)
{
	m_ids.clear();
	for (auto i: _ids)
		m_ids[i.pub()] = i.secret();
}

dev::eth::Interface* WebThreeStubServer::client() const
{
	return m_web3.ethereum();
}

std::shared_ptr<dev::shh::Interface> WebThreeStubServer::face() const
{
	return m_web3.whisper();
}

std::string WebThreeStubServer::account()
{
	if (!m_accounts.empty())
		return toJS(m_accounts.begin()->first);
	return "";
}

Json::Value WebThreeStubServer::accounts()
{
	Json::Value ret(Json::arrayValue);
	for (auto i: m_accounts)
		ret.append(toJS(i.first));
	return ret;
}

std::string WebThreeStubServer::addToGroup(std::string const& _group, std::string const& _who)
{
	(void)_group;
	(void)_who;
	return "";
}

std::string WebThreeStubServer::balanceAt(string const& _address)
{
	int block = 0;
	return toJS(client()->balanceAt(jsToAddress(_address), block));
}

Json::Value WebThreeStubServer::blockByHash(std::string const& _hash)
{
	if (!client())
		return "";
	return toJson(client()->blockInfo(jsToFixed<32>(_hash)));
}

Json::Value WebThreeStubServer::blockByNumber(int const& _number)
{
	if (!client())
		return "";
	return toJson(client()->blockInfo(client()->hashFromNumber(_number)));
}

static TransactionSkeleton toTransaction(Json::Value const& _json)
{
	TransactionSkeleton ret;
	if (!_json.isObject() || _json.empty())
		return ret;

	if (!_json["from"].empty())
		ret.from = jsToAddress(_json["from"].asString());
	if (!_json["to"].empty())
		ret.to = jsToAddress(_json["to"].asString());
	if (!_json["value"].empty())
		ret.value = jsToU256(_json["value"].asString());
	if (!_json["gas"].empty())
		ret.gas = jsToU256(_json["gas"].asString());
	if (!_json["gasPrice"].empty())
		ret.gasPrice = jsToU256(_json["gasPrice"].asString());
	
	if (!_json["data"].empty() || _json["code"].empty() || _json["dataclose"].empty())
	{
		if (_json["data"].isString())
			ret.data = jsToBytes(_json["data"].asString());
		else if (_json["code"].isString())
			ret.data = jsToBytes(_json["code"].asString());
		else if (_json["data"].isArray())
			for (auto i: _json["data"])
				dev::operator +=(ret.data, asBytes(jsPadded(i.asString(), 32)));
		else if (_json["code"].isArray())
			for (auto i: _json["code"])
				dev::operator +=(ret.data, asBytes(jsPadded(i.asString(), 32)));
		else if (_json["dataclose"].isArray())
			for (auto i: _json["dataclose"])
				dev::operator +=(ret.data, jsToBytes(i.asString()));
	}
	return ret;
}

std::string WebThreeStubServer::call(Json::Value const& _json)
{
	std::string ret;
	if (!client())
		return ret;
	TransactionSkeleton t = toTransaction(_json);
	if (!t.from && m_accounts.size())
	{
		auto b = m_accounts.begin()->first;
		for (auto a: m_accounts)
			if (client()->balanceAt(a.first) > client()->balanceAt(b))
				b = a.first;
		t.from = b;
	}
	if (!m_accounts.count(t.from))
		return ret;
	if (!t.gasPrice)
		t.gasPrice = 10 * dev::eth::szabo;
	if (!t.gas)
		t.gas = min<u256>(client()->gasLimitRemaining(), client()->balanceAt(t.from) / t.gasPrice);
	ret = toJS(client()->call(m_accounts[t.from].secret(), t.value, t.to, t.data, t.gas, t.gasPrice));
	return ret;
}

bool WebThreeStubServer::changed(int const& _id)
{
	if (!client())
		return false;
	return client()->checkWatch(_id);
}

std::string WebThreeStubServer::codeAt(string const& _address)
{
	int block = 0;
	return client() ? jsFromBinary(client()->codeAt(jsToAddress(_address), block)) : "";
}

std::string WebThreeStubServer::coinbase()
{
	return client() ? toJS(client()->address()) : "";
}

double WebThreeStubServer::countAt(string const& _address)
{
	int block = 0;
	return client() ? (double)(uint64_t)client()->countAt(jsToAddress(_address), block) : 0;
}

int WebThreeStubServer::defaultBlock()
{
	return client() ? client()->getDefault() : 0;
}

std::string WebThreeStubServer::gasPrice()
{
	return toJS(10 * dev::eth::szabo);
}

std::string WebThreeStubServer::get(std::string const& _name, std::string const& _key)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	string ret;
	m_db->Get(m_readOptions, ldb::Slice((char const*)k.data(), k.size()), &ret);
	return toJS(dev::asBytes(ret));
}

Json::Value WebThreeStubServer::getMessages(int const& _id)
{
	if (!client())
		return  Json::Value();
	return toJson(client()->messages(_id));
}

std::string WebThreeStubServer::getString(std::string const& _name, std::string const& _key)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	string ret;
	m_db->Get(m_readOptions, ldb::Slice((char const*)k.data(), k.size()), &ret);
	return ret;
}

bool WebThreeStubServer::haveIdentity(std::string const& _id)
{
	return m_ids.count(jsToPublic(_id)) > 0;
}

bool WebThreeStubServer::listening()
{
	return m_web3.isNetworkStarted();
}

bool WebThreeStubServer::mining()
{
	return client() ? client()->isMining() : false;
}

int WebThreeStubServer::newFilter(Json::Value const& _json)
{
	unsigned ret = -1;
	if (!client())
		return ret;
	ret = client()->installWatch(toMessageFilter(_json));
	return ret;
}

int WebThreeStubServer::newFilterString(std::string const& _filter)
{
	unsigned ret = -1;
	if (!client())
		return ret;
	if (_filter.compare("chain") == 0)
		ret = client()->installWatch(dev::eth::ChainChangedFilter);
	else if (_filter.compare("pending") == 0)
		ret = client()->installWatch(dev::eth::PendingChangedFilter);
	return ret;
}

std::string WebThreeStubServer::newGroup(std::string const& _id, std::string const& _who)
{
	(void)_id;
	(void)_who;
	return "";
}

std::string WebThreeStubServer::newIdentity()
{
	KeyPair kp = KeyPair::create();
	m_ids[kp.pub()] = kp.secret();
	return toJS(kp.pub());
}

std::string WebThreeStubServer::compile(string const& _s)
{
	return toJS(dev::eth::compileLLL(_s));
}

int WebThreeStubServer::number()
{
	return client() ? client()->number() + 1 : 0;
}

int WebThreeStubServer::peerCount()
{
	return m_web3.peerCount();
}

bool WebThreeStubServer::post(Json::Value const& _json)
{
	shh::Message m = toMessage(_json);
	Secret from;

	if (m.from() && m_ids.count(m.from()))
	{
		cwarn << "Silently signing message from identity" << m.from().abridged() << ": User validation hook goes here.";
		// TODO: insert validification hook here.
		from = m_ids[m.from()];
	}
	
	face()->inject(toSealed(_json, m, from));
	return true;
}

bool WebThreeStubServer::put(std::string const& _name, std::string const& _key, std::string const& _value)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	bytes v = jsToBytes(_value);
	m_db->Put(m_writeOptions, ldb::Slice((char const*)k.data(), k.size()), ldb::Slice((char const*)v.data(), v.size()));
	return true;
}

bool WebThreeStubServer::putString(std::string const& _name, std::string const& _key, std::string const& _value)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	string v = _value;
	m_db->Put(m_writeOptions, ldb::Slice((char const*)k.data(), k.size()), ldb::Slice((char const*)v.data(), v.size()));
	return true;
}

bool WebThreeStubServer::setCoinbase(std::string const& _address)
{
	if (!client())
		return false;
	client()->setAddress(jsToAddress(_address));
	return true;
}

bool WebThreeStubServer::setDefaultBlock(int const& _block)
{
	if (!client())
		return false;
	client()->setDefault(_block);
	return true;
}

bool WebThreeStubServer::setListening(bool const& _listening)
{
	if (_listening)
		m_web3.startNetwork();
	else
		m_web3.stopNetwork();
	return true;
}

bool WebThreeStubServer::setMining(bool const& _mining)
{
	if (!client())
		return false;

	if (_mining)
		client()->startMining();
	else
		client()->stopMining();
	return true;
}

Json::Value WebThreeStubServer::shhChanged(int const& _id)
{
	Json::Value ret(Json::arrayValue);
	auto pub = m_shhWatches[_id];
	if (!pub || m_ids.count(pub))
		for (h256 const& h: face()->checkWatch(_id))
		{
			auto e = face()->envelope(h);
			shh::Message m;
			if (pub)
			{
				cwarn << "Silently decrypting message from identity" << pub.abridged() << ": User validation hook goes here.";
				m = e.open(m_ids[pub]);
				if (!m)
					continue;
			}
			else
				m = e.open();
			ret.append(toJson(h,e,m));
		}
	
	return ret;
}

int WebThreeStubServer::shhNewFilter(Json::Value const& _json)
{
	auto w = toWatch(_json);
	auto ret = face()->installWatch(w.first);
	m_shhWatches.insert(make_pair(ret, w.second));
	return ret;
}

bool WebThreeStubServer::shhUninstallFilter(int const& _id)
{
	face()->uninstallWatch(_id);
	return true;
}

std::string WebThreeStubServer::stateAt(string const& _address, string const& _storage)
{
	int block = 0;
	return client() ? toJS(client()->stateAt(jsToAddress(_address), jsToU256(_storage), block)) : "";
}

std::string WebThreeStubServer::transact(Json::Value const& _json)
{
	std::string ret;
	if (!client())
		return ret;
	TransactionSkeleton t = toTransaction(_json);
	if (!t.from && m_accounts.size())
	{
		auto b = m_accounts.begin()->first;
		for (auto a: m_accounts)
			if (client()->balanceAt(a.first) > client()->balanceAt(b))
				b = a.first;
		t.from = b;
	}
	if (!m_accounts.count(t.from))
		return ret;
	if (!t.gasPrice)
		t.gasPrice = 10 * dev::eth::szabo;
	if (!t.gas)
		t.gas = min<u256>(client()->gasLimitRemaining(), client()->balanceAt(t.from) / t.gasPrice);
	cwarn << "Silently signing transaction from address" << t.from.abridged() << ": User validation hook goes here.";
	if (t.to)
		// TODO: from qethereum, insert validification hook here.
		client()->transact(m_accounts[t.from].secret(), t.value, t.to, t.data, t.gas, t.gasPrice);
	else
		ret = toJS(client()->transact(m_accounts[t.from].secret(), t.value, t.data, t.gas, t.gasPrice));
	client()->flushTransactions();
	return ret;
}

Json::Value WebThreeStubServer::transactionByHash(std::string const& _hash, int const& _i)
{
	if (!client())
		return "";
	return toJson(client()->transaction(jsToFixed<32>(_hash), _i));
}

Json::Value WebThreeStubServer::transactionByNumber(int const& _number, int const& _i)
{
	if (!client())
		return "";
	return toJson(client()->transaction(client()->hashFromNumber(_number), _i));
}

Json::Value WebThreeStubServer::uncleByHash(std::string const& _hash, int const& _i)
{
	if (!client())
		return "";
	return toJson(client()->uncle(jsToFixed<32>(_hash), _i));
}

Json::Value WebThreeStubServer::uncleByNumber(int const& _number, int const& _i)
{
	if (!client())
		return "";
	return toJson(client()->uncle(client()->hashFromNumber(_number), _i));
}

bool WebThreeStubServer::uninstallFilter(int const& _id)
{
	if (!client())
		return false;
	client()->uninstallWatch(_id);
	return true;
}

