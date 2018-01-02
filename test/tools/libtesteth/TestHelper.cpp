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
/** @file
 * Helper functions to work with json::spirit and test files
 */

#include "TestHelper.h"
#include "Options.h"
#include "TestOutputHelper.h"

#include <BuildInfo.h>
#include <libethashseal/EthashCPUMiner.h>
#include <libethereum/Client.h>

#include <yaml-cpp/yaml.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/path.hpp>
#include <set>
#include <string>

using namespace std;
using namespace dev::eth;
namespace fs = boost::filesystem;

namespace dev
{
namespace eth
{

void mine(Client& c, int numBlocks)
{
    auto startBlock = c.blockChain().details().number;

    c.startSealing();
    while (c.blockChain().details().number < startBlock + numBlocks)
        this_thread::sleep_for(chrono::milliseconds(100));
    c.stopSealing();
}

void connectClients(Client& c1, Client& c2)
{
    (void)c1;
    (void)c2;
// TODO: Move to WebThree. eth::Client no longer handles networking.
#if 0
	short c1Port = 20000;
	short c2Port = 21000;
	c1.startNetwork(c1Port);
	c2.startNetwork(c2Port);
	c2.connect("127.0.0.1", c1Port);
#endif
}

void mine(Block& s, BlockChain const& _bc, SealEngineFace* _sealer)
{
    EthashCPUMiner::setNumInstances(1);
    s.commitToSeal(_bc, s.info().extraData());
    Notified<bytes> sealed;
    _sealer->onSealGenerated([&](bytes const& sealedHeader) { sealed = sealedHeader; });
    _sealer->generateSeal(s.info());
    sealed.waitNot({});
    _sealer->onSealGenerated([](bytes const&) {});
    s.sealBlock(sealed);
}

void mine(BlockHeader& _bi, SealEngineFace* _sealer, bool _verify)
{
    Notified<bytes> sealed;
    _sealer->onSealGenerated([&](bytes const& sealedHeader) { sealed = sealedHeader; });
    _sealer->generateSeal(_bi);
    sealed.waitNot({});
    _sealer->onSealGenerated([](bytes const&) {});
    _bi = BlockHeader(sealed, HeaderData);
    //	cdebug << "Block mined" << Ethash::boundary(_bi).hex() <<
    // Ethash::nonce(_bi) << _bi.hash(WithoutSeal).hex();
    if (_verify)  // sometimes it is needed to mine incorrect blockheaders for
                  // testing
        _sealer->verify(JustSeal, _bi);
}

}

namespace test
{

string netIdToString(eth::Network _netId)
{
    switch (_netId)
    {
    case eth::Network::FrontierTest:
        return "Frontier";
    case eth::Network::HomesteadTest:
        return "Homestead";
    case eth::Network::EIP150Test:
        return "EIP150";
    case eth::Network::EIP158Test:
        return "EIP158";
    case eth::Network::ByzantiumTest:
        return "Byzantium";
    case eth::Network::ConstantinopleTest:
        return "Constantinople";
    case eth::Network::FrontierToHomesteadAt5:
        return "FrontierToHomesteadAt5";
    case eth::Network::HomesteadToDaoAt5:
        return "HomesteadToDaoAt5";
    case eth::Network::HomesteadToEIP150At5:
        return "HomesteadToEIP150At5";
    case eth::Network::EIP158ToByzantiumAt5:
        return "EIP158ToByzantiumAt5";
    case eth::Network::TransitionnetTest:
        return "TransitionNet";
    default:
        return "other";
    }
    return "unknown";
}

eth::Network stringToNetId(string const& _netname)
{
    // Networks that used in .json tests
    static vector<eth::Network> const networks{
        {eth::Network::FrontierTest, eth::Network::HomesteadTest, eth::Network::EIP150Test,
            eth::Network::EIP158Test, eth::Network::ByzantiumTest, eth::Network::ConstantinopleTest,
            eth::Network::FrontierToHomesteadAt5, eth::Network::HomesteadToDaoAt5,
            eth::Network::HomesteadToEIP150At5, eth::Network::EIP158ToByzantiumAt5,
            eth::Network::TransitionnetTest}};

    for (auto const& net : networks)
        if (netIdToString(net) == _netname)
            return net;

    BOOST_ERROR(TestOutputHelper::get().testName() + " network not found: " + _netname);
    return eth::Network::FrontierTest;
}

bool isDisabledNetwork(eth::Network _net)
{
    Options const& opt = Options::get();
    if (opt.all || opt.filltests || opt.createRandomTest || !opt.singleTestNet.empty())
    {
        if (_net == eth::Network::ConstantinopleTest)
            return true;
        return false;
    }
    switch (_net)
    {
    case eth::Network::FrontierTest:
    case eth::Network::HomesteadTest:
    case eth::Network::ConstantinopleTest:
    case eth::Network::FrontierToHomesteadAt5:
    case eth::Network::HomesteadToDaoAt5:
    case eth::Network::HomesteadToEIP150At5:
        return true;
    default:
        break;
    }
    return false;
}

vector<eth::Network> const& getNetworks()
{
    // Networks for the test case execution when filling the tests
    static vector<eth::Network> const networks{{eth::Network::FrontierTest,
        eth::Network::HomesteadTest, eth::Network::EIP150Test, eth::Network::EIP158Test,
        eth::Network::ByzantiumTest, eth::Network::ConstantinopleTest}};
    return networks;
}

string exportLog(eth::LogEntries const& _logs)
{
    RLPStream s;
    s.appendList(_logs.size());
    for (LogEntry const& l : _logs)
        l.streamRLP(s);
    return toHexPrefixed(sha3(s.out()));
}

u256 toInt(json_spirit::mValue const& _v)
{
    switch (_v.type())
    {
    case json_spirit::str_type:
        return u256(_v.get_str());
    case json_spirit::int_type:
        return (u256)_v.get_uint64();
    case json_spirit::bool_type:
        return (u256)(uint64_t)_v.get_bool();
    case json_spirit::real_type:
        return (u256)(uint64_t)_v.get_real();
    default:
        cwarn << "Bad type for scalar: " << _v.type();
    }
    return 0;
}

byte toByte(json_spirit::mValue const& _v)
{
    switch (_v.type())
    {
    case json_spirit::str_type:
        return (byte)stoi(_v.get_str());
    case json_spirit::int_type:
        return (byte)_v.get_uint64();
    case json_spirit::bool_type:
        return (byte)_v.get_bool();
    case json_spirit::real_type:
        return (byte)_v.get_real();
    default:
        cwarn << "Bad type for scalar: " << _v.type();
    }
    return 0;
}

bytes importByteArray(string const& _str)
{
    checkHexHasEvenLength(_str);
    return fromHex(_str.substr(0, 2) == "0x" ? _str.substr(2) : _str, WhenError::Throw);
}

bytes importData(json_spirit::mObject const& _o)
{
    bytes data;
    if (_o.at("data").type() == json_spirit::str_type)
        data = importByteArray(replaceLLL(_o.at("data").get_str()));
    else
        for (auto const& j : _o.at("data").get_array())
            data.push_back(toByte(replaceLLL(j.get_str())));
    return data;
}

string replaceLLL(string const& _code)
{
    string compiledCode;
    compiledCode = compileLLL(_code);
    if (_code.size() > 0)
        BOOST_REQUIRE_MESSAGE(compiledCode.size() > 0,
            "Bytecode is missing! '" + _code + "' " + TestOutputHelper::get().testName());
    return compiledCode;
}

void replaceLLLinState(json_spirit::mObject& _o)
{
    json_spirit::mObject& fieldsObj = _o.count("alloc") ?
                                          _o["alloc"].get_obj() :
                                          _o.count("accounts") ? _o["accounts"].get_obj() : _o;
    for (auto& account : fieldsObj)
    {
        auto obj = account.second.get_obj();
        if (obj.count("code") && obj["code"].type() == json_spirit::str_type)
            obj["code"] = replaceLLL(obj["code"].get_str());
        account.second = obj;
	}
}

vector<fs::path> getFiles(
    fs::path const& _dirPath, set<string> const _extentionMask, string const& _particularFile)
{
    vector<fs::path> files;
    for (auto const& ext : _extentionMask)
    {
        if (!_particularFile.empty())
        {
            fs::path file = _dirPath / (_particularFile + ext);
            if (fs::exists(file))
                files.push_back(file);
        }
        else
        {
            using fsIterator = fs::directory_iterator;
            for (fsIterator it(_dirPath); it != fsIterator(); ++it)
            {
                if (fs::is_regular_file(it->path()) && it->path().extension() == ext)
                    files.push_back(it->path());
            }
        }
    }
    return files;
}

json_spirit::mValue convertYamlNodeToJson(YAML::Node _node)
{
    if (_node.IsNull())
        return json_spirit::mValue();

    if (_node.IsScalar())
    {
        if (_node.Tag() == "tag:yaml.org,2002:int")
            return _node.as<int>();
        else
            return _node.as<string>();
    }

    if (_node.IsMap())
    {
        json_spirit::mObject jObject;
        for (auto const& i : _node)
            jObject.emplace(i.first.as<string>(), convertYamlNodeToJson(i.second));
        return jObject;
    }

    if (_node.IsSequence())
    {
        json_spirit::mArray jArray;
        for (size_t i = 0; i < _node.size(); i++)
            jArray.emplace_back(convertYamlNodeToJson(_node[i]));
        return jArray;
    }

    BOOST_ERROR("Error parsing YAML node. Element type not defined!");
    return json_spirit::mValue();
}

/// this function is here so not to include <YAML.h> in other .cpp files
json_spirit::mValue parseYamlToJson(string const& _string)
{
    YAML::Node testFile = YAML::Load(_string);
    return convertYamlNodeToJson(testFile);
}

string executeCmd(string const& _command)
{
#if defined(_WIN32)
    BOOST_ERROR("executeCmd() has not been implemented for Windows.");
    return "";
#else
    string out;
    char output[1024];
    FILE* fp = popen(_command.c_str(), "r");
    if (fp == NULL)
        BOOST_ERROR("Failed to run " + _command);
    if (fgets(output, sizeof(output) - 1, fp) == NULL)
        BOOST_ERROR("Reading empty result for " + _command);
    else
    {
        while (true)
        {
            out += string(output);
            if (fgets(output, sizeof(output) - 1, fp) == NULL)
                break;
        }
    }

    int exitCode = pclose(fp);
    if (exitCode != 0)
        BOOST_ERROR("The command '" + _command + "' exited with " + toString(exitCode) + " code.");
    return boost::trim_copy(out);
#endif
}

string compileLLL(string const& _code)
{
    if (_code == "")
        return "0x";
    if (_code.substr(0, 2) == "0x" && _code.size() >= 2)
    {
        checkHexHasEvenLength(_code);
        return _code;
    }

#if defined(_WIN32)
    BOOST_ERROR("LLL compilation only supported on posix systems.");
    return "";
#else
    fs::path path(fs::temp_directory_path() / fs::unique_path());
    string cmd = string("lllc ") + path.string();
    writeFile(path.string(), _code);
    string result = executeCmd(cmd);
    fs::remove(path);
    result = "0x" + result;
    checkHexHasEvenLength(result);
    return result;
#endif
}

void checkHexHasEvenLength(string const& _str)
{
    if (_str.size() % 2)
        BOOST_ERROR(TestOutputHelper::get().testName() +
                    " An odd-length hex string represents a byte sequence: " + _str);
}

bytes importCode(json_spirit::mObject const& _o)
{
    bytes code;
    if (_o.count("code") == 0)
        return code;
    if (_o.at("code").type() == json_spirit::str_type)
        if (_o.at("code").get_str().find("0x") != 0)
            code = fromHex(compileLLL(_o.at("code").get_str()));
        else
            code = importByteArray(_o.at("code").get_str());
    else if (_o.at("code").type() == json_spirit::array_type)
    {
        code.clear();
        for (auto const& j : _o.at("code").get_array())
            code.push_back(toByte(j));
    }
    return code;
}

LogEntries importLog(json_spirit::mArray const& _a)
{
    LogEntries logEntries;
    for (auto const& l : _a)
    {
        json_spirit::mObject o = l.get_obj();
        BOOST_REQUIRE(o.count("address") > 0);
        BOOST_REQUIRE(o.count("topics") > 0);
        BOOST_REQUIRE(o.count("data") > 0);
        BOOST_REQUIRE(o.count("bloom") > 0);
        LogEntry log;
        log.address = Address(o.at("address").get_str());
        for (auto const& t : o.at("topics").get_array())
            log.topics.push_back(h256(t.get_str()));
        log.data = importData(o);
        logEntries.push_back(log);
    }
    return logEntries;
}

void checkOutput(bytesConstRef _output, json_spirit::mObject const& _o)
{
    int j = 0;
    auto expectedOutput = _o.at("out").get_str();

    if (expectedOutput.find("#") == 0)
        BOOST_CHECK(_output.size() == toInt(expectedOutput.substr(1)));
    else if (_o.at("out").type() == json_spirit::array_type)
        for (auto const& d : _o.at("out").get_array())
        {
            BOOST_CHECK_MESSAGE(_output[j] == toInt(d), "Output byte [" << j << "] different!");
            ++j;
        }
    else if (expectedOutput.find("0x") == 0)
        BOOST_CHECK(_output.contentsEqual(fromHex(expectedOutput.substr(2))));
    else
        BOOST_CHECK(_output.contentsEqual(fromHex(expectedOutput)));
}

void checkStorage(map<u256, u256> const& _expectedStore, map<u256, u256> const& _resultStore,
    Address const& _expectedAddr)
{
    for (auto&& expectedStorePair : _expectedStore)
    {
        auto& expectedStoreKey = expectedStorePair.first;
        auto resultStoreIt = _resultStore.find(expectedStoreKey);
        if (resultStoreIt == _resultStore.end())
            BOOST_ERROR(_expectedAddr << ": missing store key " << expectedStoreKey);
        else
        {
            auto& expectedStoreValue = expectedStorePair.second;
            auto& resultStoreValue = resultStoreIt->second;
            BOOST_CHECK_MESSAGE(expectedStoreValue == resultStoreValue,
                _expectedAddr << ": store[" << expectedStoreKey << "] = " << resultStoreValue
                              << ", expected " << expectedStoreValue);
        }
    }
    BOOST_CHECK_EQUAL(_resultStore.size(), _expectedStore.size());
    for (auto&& resultStorePair : _resultStore)
    {
        if (!_expectedStore.count(resultStorePair.first))
            BOOST_ERROR(_expectedAddr << ": unexpected store key " << resultStorePair.first);
    }
}

void checkCallCreates(
    eth::Transactions const& _resultCallCreates, eth::Transactions const& _expectedCallCreates)
{
    BOOST_REQUIRE_EQUAL(_resultCallCreates.size(), _expectedCallCreates.size());

    for (size_t i = 0; i < _resultCallCreates.size(); ++i)
    {
        BOOST_CHECK(_resultCallCreates[i].data() == _expectedCallCreates[i].data());
        BOOST_CHECK(
            _resultCallCreates[i].receiveAddress() == _expectedCallCreates[i].receiveAddress());
        BOOST_CHECK(_resultCallCreates[i].gas() == _expectedCallCreates[i].gas());
        BOOST_CHECK(_resultCallCreates[i].value() == _expectedCallCreates[i].value());
    }
}

string prepareVersionString()
{
    // cpp-1.3.0+commit.6be76b64.Linux.g++
    string commit(DEV_QUOTED(ETH_COMMIT_HASH));
    string version = "cpp-" + string(ETH_PROJECT_VERSION);
    version += "+commit." + commit.substr(0, 8);
    version +=
        "." + string(DEV_QUOTED(ETH_BUILD_OS)) + "." + string(DEV_QUOTED(ETH_BUILD_COMPILER));
    return version;
}

string prepareLLLCVersionString()
{
    string result = test::executeCmd("lllc --version");
    string::size_type pos = result.rfind("Version");
    if (pos != string::npos)
        return result.substr(pos, result.length());
    return "Error getting LLLC Version";
}

void copyFile(fs::path const& _source, fs::path const& _destination)
{
    fs::ifstream src(_source, ios::binary);
    fs::ofstream dst(_destination, ios::binary);
    dst << src.rdbuf();
}

RLPStream createRLPStreamFromTransactionFields(json_spirit::mObject const& _tObj)
{
    // Construct Rlp of the given transaction
    RLPStream rlpStream;
    rlpStream.appendList(_tObj.size());

    if (_tObj.count("nonce"))
        rlpStream << bigint(_tObj.at("nonce").get_str());

    if (_tObj.count("gasPrice"))
        rlpStream << bigint(_tObj.at("gasPrice").get_str());

    if (_tObj.count("gasLimit"))
        rlpStream << bigint(_tObj.at("gasLimit").get_str());

    if (_tObj.count("to"))
    {
        if (_tObj.at("to").get_str().empty())
            rlpStream << "";
        else
            rlpStream << importByteArray(_tObj.at("to").get_str());
    }

    if (_tObj.count("value"))
        rlpStream << bigint(_tObj.at("value").get_str());

    if (_tObj.count("data"))
        rlpStream << importData(_tObj);

    if (_tObj.count("v"))
        rlpStream << bigint(_tObj.at("v").get_str());

    if (_tObj.count("r"))
        rlpStream << bigint(_tObj.at("r").get_str());

    if (_tObj.count("s"))
        rlpStream << bigint(_tObj.at("s").get_str());

    if (_tObj.count("extrafield"))
        rlpStream << bigint(_tObj.at("extrafield").get_str());

    return rlpStream;
}

dev::eth::BlockHeader constructHeader(h256 const& _parentHash, h256 const& _sha3Uncles,
    Address const& _author, h256 const& _stateRoot, h256 const& _transactionsRoot,
    h256 const& _receiptsRoot, dev::eth::LogBloom const& _logBloom, u256 const& _difficulty,
    u256 const& _number, u256 const& _gasLimit, u256 const& _gasUsed, u256 const& _timestamp,
    bytes const& _extraData)
{
    RLPStream rlpStream;
    rlpStream.appendList(15);

    rlpStream << _parentHash << _sha3Uncles << _author << _stateRoot << _transactionsRoot
              << _receiptsRoot << _logBloom << _difficulty << _number << _gasLimit << _gasUsed
              << _timestamp << _extraData << h256{} << eth::Nonce{};

    return BlockHeader(rlpStream.out(), HeaderData);
}

void updateEthashSeal(dev::eth::BlockHeader& _header, h256 const& _mixHash, h64 const& _nonce)
{
    Ethash::setNonce(_header, _nonce);
    Ethash::setMixHash(_header, _mixHash);
}

namespace
{
Listener* g_listener;
}

void Listener::registerListener(Listener& _listener)
{
    g_listener = &_listener;
}

void Listener::notifySuiteStarted(string const& _name)
{
    if (g_listener)
        g_listener->suiteStarted(_name);
}

void Listener::notifyTestStarted(string const& _name)
{
    if (g_listener)
        g_listener->testStarted(_name);
}

void Listener::notifyTestFinished(int64_t _gasUsed)
{
    if (g_listener)
        g_listener->testFinished(_gasUsed);
}
}
}  // namespaces
