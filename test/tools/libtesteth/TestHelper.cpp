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
#include "wast2wasm.h"

#include <libdevcore/JsonUtils.h>
#include <libethashseal/EthashCPUMiner.h>
#include <libethereum/Client.h>

#include <aleth/buildinfo.h>

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
void mine(Client& _c, int _numBlocks)
{
    int sealedBlocks = 0;
    auto sealHandler = _c.setOnBlockSealed([_numBlocks, &sealedBlocks, &_c](bytes const&) {
        if (++sealedBlocks == _numBlocks)
            _c.stopSealing();
    });

    int importedBlocks = 0;
    std::promise<void> allBlocksImported;
    auto chainChangedHandler = _c.setOnChainChanged(
        [_numBlocks, &importedBlocks, &allBlocksImported](h256s const&, h256s const& _newBlocks) {
            importedBlocks += _newBlocks.size();
            if (importedBlocks == _numBlocks)
                allBlocksImported.set_value();
        });

    _c.startSealing();
    allBlocksImported.get_future().wait();
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
        _sealer->verify(CheckNothingNew, _bi);
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
    case eth::Network::ConstantinopleFixTest:
        return "ConstantinopleFix";
    case eth::Network::FrontierToHomesteadAt5:
        return "FrontierToHomesteadAt5";
    case eth::Network::HomesteadToDaoAt5:
        return "HomesteadToDaoAt5";
    case eth::Network::HomesteadToEIP150At5:
        return "HomesteadToEIP150At5";
    case eth::Network::EIP158ToByzantiumAt5:
        return "EIP158ToByzantiumAt5";
    case eth::Network::ByzantiumToConstantinopleFixAt5:
        return "ByzantiumToConstantinopleFixAt5";
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
            eth::Network::ConstantinopleFixTest, eth::Network::FrontierToHomesteadAt5,
            eth::Network::HomesteadToDaoAt5, eth::Network::HomesteadToEIP150At5,
            eth::Network::EIP158ToByzantiumAt5, eth::Network::ByzantiumToConstantinopleFixAt5,
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
        return false;
    }
    switch (_net)
    {
    case eth::Network::FrontierTest:
    case eth::Network::HomesteadTest:
    case eth::Network::FrontierToHomesteadAt5:
    case eth::Network::HomesteadToDaoAt5:
    case eth::Network::HomesteadToEIP150At5:
    case eth::Network::ConstantinopleTest:  // Disable initial constantinople version
        return true;
    default:
        break;
    }
    return false;
}

set<eth::Network> const& getNetworks()
{
    // Networks for the test case execution when filling the tests
    static set<eth::Network> const networks{
        {eth::Network::FrontierTest, eth::Network::HomesteadTest, eth::Network::EIP150Test,
            eth::Network::EIP158Test, eth::Network::ByzantiumTest, eth::Network::ConstantinopleTest,
            eth::Network::ConstantinopleFixTest}};
    return networks;
}

/// translate network names in expect section field
/// >Homestead to EIP150, EIP158, Byzantium, ...
/// <=Homestead to Frontier, Homestead
set<string> translateNetworks(set<string> const& _networks)
{
    // construct vector with test network names in a right order (from Frontier to Homestead ... to
    // Constantinople)
    vector<string> forks;
    for (auto const& net : getNetworks())
        forks.push_back(test::netIdToString(net));

    set<string> out;
    for (auto const& net : _networks)
    {
        bool isNetworkTranslated = false;
        string possibleNet = net.substr(1, net.length() - 1);
        vector<string>::iterator it = std::find(forks.begin(), forks.end(), possibleNet);

        if (it != forks.end() && net.size() > 1)
        {
            if (net[0] == '>')
            {
                while (++it != forks.end())
                {
                    out.emplace(*it);
                    isNetworkTranslated = true;
                }
            }
            else if (net[0] == '<')
            {
                while (it != forks.begin())
                {
                    out.emplace(*(--it));
                    isNetworkTranslated = true;
                }
            }
        }

        possibleNet = net.substr(2, net.length() - 2);
        it = std::find(forks.begin(), forks.end(), possibleNet);
        if (it != forks.end() && net.size() > 2)
        {
            if (net[0] == '>' && net[1] == '=')
            {
                while (it != forks.end())
                {
                    out.emplace(*(it++));
                    isNetworkTranslated = true;
                }
            }
            else if (net[0] == '<' && net[1] == '=')
            {
                out.emplace(*it);
                isNetworkTranslated = true;
                while (it != forks.begin())
                    out.emplace(*(--it));
            }
        }

        // if nothing has been inserted, just push the untranslated network as is
        if (!isNetworkTranslated)
        {
            ImportTest::checkAllowedNetwork(net);
            out.emplace(net);
        }
    }
    return out;
}

string exportLog(eth::LogEntries const& _logs)
{
    RLPStream s;
    s.appendList(_logs.size());
    for (LogEntry const& l : _logs)
        l.streamRLP(s);
    return toHexPrefixed(sha3(s.out()));
}

u256 toU256(json_spirit::mValue const& _v)
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

int64_t toInt64(json_spirit::mValue const& _v)
{
    int64_t n = 0;
    switch (_v.type())
    {
    case json_spirit::str_type:
        n = std::stoll(_v.get_str(), nullptr, 0);
        break;
    case json_spirit::int_type:
        n = _v.get_int64();
        break;
    default:
        cwarn << "Bad type for scalar: " << _v.type();
    }
    return n;
}

uint64_t toUint64(json_spirit::mValue const& _v)
{
    uint64_t n = 0;
    switch (_v.type())
    {
    case json_spirit::str_type:
    {
        long long readval = std::stoll(_v.get_str(), nullptr, 0);
        if (readval < 0)
            BOOST_THROW_EXCEPTION(UnexpectedNegative() << errinfo_comment(
                                      "TestOutputHelper::toUint64: unexpected negative value: " +
                                      std::to_string(readval)));
        n = readval;
    }
    break;
    case json_spirit::int_type:
        n = _v.get_uint64();
        break;
    default:
        cwarn << "Bad type for scalar: " << _v.type();
    }
    return n;
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

bytes processDataOrCode(json_spirit::mObject const& _o, string const& nodeName)
{
    bytes ret;
    if (_o.count(nodeName) == 0)
        return bytes();
    if (_o.at(nodeName).type() == json_spirit::str_type)
        if (_o.at(nodeName).get_str().find("0x") != 0)
            ret = fromHex(replaceCode(_o.at(nodeName).get_str()));
        else
            ret = importByteArray(_o.at(nodeName).get_str());
    else if (_o.at(nodeName).type() == json_spirit::array_type)
    {
        for (auto const& j : _o.at(nodeName).get_array())
            ret.push_back(toByte(j));
    }
    return ret;
}

bytes importData(json_spirit::mObject const& _o)
{
    return processDataOrCode(_o, "data");
}

string replaceCode(string const& _code)
{
    if (_code == "")
        return "0x";
    if (_code.substr(0, 2) == "0x" && _code.size() >= 2)
    {
        checkHexHasEvenLength(_code);
        return _code;
    }
    if (_code.find("(module") == 0)
        return wast2wasm(_code);

    string compiledCode = compileLLL(_code);
    if (_code.size() > 0)
        BOOST_REQUIRE_MESSAGE(compiledCode.size() > 0,
            "Bytecode is missing! '" + _code + "' " + TestOutputHelper::get().testName());
    return compiledCode;
}

void replaceCodeInState(json_spirit::mObject& _o)
{
    json_spirit::mObject& fieldsObj = _o.count("alloc") ?
                                          _o["alloc"].get_obj() :
                                          _o.count("accounts") ? _o["accounts"].get_obj() : _o;
    for (auto& account : fieldsObj)
    {
        auto obj = account.second.get_obj();
        if (obj.count("code") && obj["code"].type() == json_spirit::str_type)
            obj["code"] = replaceCode(obj["code"].get_str());
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
#if defined(_WIN32)
    BOOST_ERROR("LLL compilation only supported on posix systems.");
    return "";
#else
    fs::path path(fs::temp_directory_path() / fs::unique_path());
    writeFile(path.string(), _code);
    // NOTE: this will abort if execution failed
    string result = executeCmd(string("lllc ") + path.string());
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
    return processDataOrCode(_o, "code");
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
        BOOST_CHECK(_output.size() == toU256(expectedOutput.substr(1)));
    else if (_o.at("out").type() == json_spirit::array_type)
        for (auto const& d : _o.at("out").get_array())
        {
            BOOST_CHECK_MESSAGE(_output[j] == toU256(d), "Output byte [" << j << "] different!");
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

void requireJsonFields(json_spirit::mObject const& _o, string const& _section,
    map<string, json_spirit::Value_type> const& _validationMap)
{
    // check for unexpected fiedls
    for (auto const field : _o)
        BOOST_CHECK_MESSAGE(_validationMap.count(field.first),
            field.first + " should not be declared in " + _section + " section!");

    // check field types with validation map
    for (auto const vmap : _validationMap)
    {
        BOOST_REQUIRE_MESSAGE(
            _o.count(vmap.first) > 0, vmap.first + " not found in " + _section + " section!");
        BOOST_REQUIRE_MESSAGE(_o.at(vmap.first).type() == vmap.second,
            _section + " " + vmap.first + " expected to be " + jsonTypeAsString(vmap.second) +
                ", but set to " + jsonTypeAsString(_o.at(vmap.first).type()));
    }
}

string prepareVersionString()
{
    return string{"testeth "} + aleth_get_buildinfo()->project_version;
}

string prepareLLLCVersionString()
{
    string result = test::executeCmd("lllc --version");
    string::size_type pos = result.rfind("Version");
    if (pos != string::npos)
        return result.substr(pos, result.length());
    return "Error getting LLLC Version";
}

// A simple C++ implementation of the Levenshtein distance algorithm to measure the amount of
// difference between two strings. https://gist.github.com/TheRayTracer/2644387
size_t levenshteinDistance(char const* _s, size_t _n, char const* _t, size_t _m)
{
    ++_n;
    ++_m;
    size_t* d = new size_t[_n * _m];

    memset(d, 0, sizeof(size_t) * _n * _m);
    for (size_t i = 1, im = 0; i < _m; ++i, ++im)
    {
        for (size_t j = 1, jn = 0; j < _n; ++j, ++jn)
        {
            if (_s[jn] == _t[im])
                d[(i * _n) + j] = d[((i - 1) * _n) + (j - 1)];
            else
            {
                d[(i * _n) + j] = min(d[(i - 1) * _n + j] + 1, /* A deletion. */
                    min(d[i * _n + (j - 1)] + 1,               /* An insertion. */
                        d[(i - 1) * _n + (j - 1)] + 1));       /* A substitution. */
            }
        }
    }

    size_t r = d[_n * _m - 1];
    delete[] d;
    return r;
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
    RLPStream rlpStream(15);

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
