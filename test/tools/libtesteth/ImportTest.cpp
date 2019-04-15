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
 * Helper class for managing data when running state tests
 */

#include <libevm/LegacyVM.h>
#include <libethereum/ExtVM.h>
#include <libethereum/ValidationSchemes.h>
#include <test/tools/jsontests/BlockChainTests.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/ImportTest.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtestutils/Common.h>
#include <test/tools/libtestutils/TestLastBlockHashes.h>

#include <boost/filesystem/path.hpp>

using namespace dev;
using namespace dev::test;
using namespace std;
namespace fs = boost::filesystem;

namespace
{
vector<h256> lastHashes(u256 _currentBlockNumber)
{
    vector<h256> ret;
    for (u256 i = 1; i <= 256 && i <= _currentBlockNumber; ++i)
        ret.push_back(sha3(toString(_currentBlockNumber - i)));
    return ret;
}
}

ImportTest::ImportTest(json_spirit::mObject const& _input, json_spirit::mObject& _output):
    m_statePre(0, OverlayDB(), eth::BaseState::Empty),
    m_statePost(0, OverlayDB(), eth::BaseState::Empty),
    m_testInputObject(_input),
    m_testOutputObject(_output)
{
    importEnv(_input.at("env").get_obj());
    importTransaction(_input.at("transaction").get_obj());
    importState(_input.at("pre").get_obj(), m_statePre);
}

void ImportTest::makeBlockchainTestFromStateTest(set<eth::Network> const& _networks) const
{
    // Generate blockchain test filler
    string testnameOrig = TestOutputHelper::get().testName();
    for (auto const& tr : m_transactions)
    {
        json_spirit::mObject json;
        json_spirit::mObject testObj;

        // generate the test name with transaction detail
        string postfix = "_d" + toString(tr.dataInd);
        postfix += "g" + toString(tr.gasInd);
        postfix += "v" + toString(tr.valInd);
        string const testname = testnameOrig + postfix;

        // basic genesis
        json_spirit::mObject genesisObj = TestBlockChain::defaultGenesisBlockJson();
        genesisObj["coinbase"] = toString(m_envInfo->author());
        genesisObj["gasLimit"] = toCompactHexPrefixed(m_envInfo->gasLimit());
        genesisObj["timestamp"] = toCompactHexPrefixed(m_envInfo->timestamp() - 50);
        testObj["genesisBlockHeader"] = genesisObj;
        testObj["pre"] = fillJsonWithState(m_statePre);
        if (m_testInputObject.count("_info") &&
            m_testInputObject.at("_info").get_obj().count("comment"))
        {
            json_spirit::mObject testInfoObj;
            testInfoObj["comment"] =
                m_testInputObject.at("_info").get_obj().at("comment").get_str();
            testObj["_info"] = testInfoObj;
        }

        // generate expect sections for this transaction
        BOOST_REQUIRE(m_testInputObject.count("expect") > 0);

        State s = State(0, OverlayDB(), eth::BaseState::Empty);
        AccountMaskMap m;
        StateAndMap smap{s, m};
        vector<size_t> stateIndexesToPrint;
        json_spirit::mArray expetSectionArray;

        for (auto const& net : _networks)
        {
            auto trDup = tr;
            trDup.netId = net;

            // Calculate the block reward
            ChainParams chainParams{genesisInfo(net)};
            chainParams.sealEngineName = NoProof::name();  // Disable mining
            EVMSchedule const schedule = chainParams.scheduleForBlockNumber(1);
            u256 const blockReward = chainParams.blockReward(schedule);

            TrExpectSection search{trDup, smap};
            for (auto const& exp : m_testInputObject.at("expect").get_array())
            {
                TrExpectSection* search2 = &search;
                checkGeneralTestSectionSearch(exp.get_obj(), stateIndexesToPrint, "", search2);
                if (search.second.first.addresses().size() !=
                        0)  // if match in the expect sections for this tr found
                {
                    // replace expected mining reward (in state tests it is 0)
                    json_spirit::mObject obj =
                            fillJsonWithState(search2->second.first, search2->second.second);
                    for (auto& adr : obj)
                    {
                        if (adr.first == toHexPrefixed(m_envInfo->author()) &&
                                adr.second.get_obj().count("balance"))
                        {
                            u256 expectCoinbaseBalance = toU256(adr.second.get_obj()["balance"]);
                            expectCoinbaseBalance += blockReward;
                            adr.second.get_obj()["balance"] =
                                    toCompactHexPrefixed(expectCoinbaseBalance);
                        }
                    }

                    json_spirit::mObject expetSectionObj;
                    expetSectionObj["network"] = test::netIdToString(net);
                    expetSectionObj["result"] = obj;
                    expetSectionArray.push_back(expetSectionObj);
                    break;
                }
            }  // for exp
        }      // for net

        testObj["expect"] = expetSectionArray;

        // rewrite header section for a block by the statetest parameters
        json_spirit::mObject rewriteHeader;
        rewriteHeader["gasLimit"] = toCompactHexPrefixed(m_envInfo->gasLimit());
        rewriteHeader["difficulty"] = toCompactHexPrefixed(m_envInfo->difficulty());
        rewriteHeader["timestamp"] = toCompactHexPrefixed(m_envInfo->timestamp());
        rewriteHeader["updatePoW"] = "1";

        json_spirit::mArray blocksArr;
        json_spirit::mArray transcArr;
        transcArr.push_back(fillJsonWithTransaction(tr.transaction));
        json_spirit::mObject blocksObj;
        blocksObj["blockHeaderPremine"] = rewriteHeader;
        blocksObj["transactions"] = transcArr;
        blocksObj["uncleHeaders"] = json_spirit::mArray();
        blocksArr.push_back(blocksObj);
        testObj["blocks"] = blocksArr;
        testObj["sealEngine"] = NoProof::name();
        json[testname] = testObj;

        // Write a filler file to the filler folder
        BCGeneralStateTestsSuite genSuite;
        fs::path const testFillerFile =
                genSuite.getFullPathFiller(TestOutputHelper::get().caseName()) /
                fs::path(testname + "Filler.json");
        writeFile(testFillerFile, asBytes(json_spirit::write_string((mValue)json, true)));

        // Execute test filling for this file
        genSuite.executeTest(TestOutputHelper::get().caseName(), testFillerFile);

    }  // transactions
}

/// returns all networks that are defined in all expect sections
set<eth::Network> ImportTest::getAllNetworksFromExpectSections(
        json_spirit::mArray const& _expects, testType _testType)
{
    set<string> allNetworks;
    for (auto const& exp : _expects)
    {
        if (_testType == testType::BlockchainTest)
        {
            BOOST_REQUIRE(exp.get_obj().count("network") > 0);
            if (exp.get_obj().at("network").type() == json_spirit::str_type)
                requireJsonFields(exp.get_obj(), "expect",
                {{"network", jsonVType::str_type}, {"result", jsonVType::obj_type}});
            else
                requireJsonFields(exp.get_obj(), "expect",
                {{"network", jsonVType::array_type}, {"result", jsonVType::obj_type}});
        }
        else if (_testType == testType::StateTest)
            requireJsonFields(exp.get_obj(), "expect",
            {{"indexes", jsonVType::obj_type}, {"network", jsonVType::array_type},
             {"result", jsonVType::obj_type}});
        ImportTest::parseJsonStrValueIntoSet(exp.get_obj().at("network"), allNetworks);
    }

    allNetworks = test::translateNetworks(allNetworks);
    set<eth::Network> networkSet;
    for (auto const& net : allNetworks)
        networkSet.emplace(test::stringToNetId(net));
    return networkSet;
}

bytes ImportTest::executeTest(bool _isFilling)
{
    assert(m_envInfo);
    set<eth::Network> networks;
    if (!Options::get().singleTestNet.empty())
        networks.emplace(stringToNetId(Options::get().singleTestNet));
    else if (_isFilling)
    {
        // Run tests only on networks from expect sections
        BOOST_REQUIRE(m_testInputObject.count("expect") > 0);
        networks = getAllNetworksFromExpectSections(
                    m_testInputObject.at("expect").get_array(), testType::StateTest);
    }
    else
    {
        // Run tests only on networks that are in post state of the filled test
        for (auto const& post : m_testInputObject.at("post").get_obj())
            networks.emplace(test::stringToNetId(post.first));
    }

    vector<transactionToExecute> transactionResults;
    for (auto const& net : networks)
    {
        if (isDisabledNetwork(net))
            continue;

        for (auto& tr : m_transactions)
        {
            Options const& opt = Options::get();
            if(opt.trDataIndex != -1 && opt.trDataIndex != tr.dataInd)
                continue;
            if(opt.trGasIndex != -1 && opt.trGasIndex != tr.gasInd)
                continue;
            if(opt.trValueIndex != -1 && opt.trValueIndex != tr.valInd)
                continue;

            std::tie(tr.postState, tr.output, tr.changeLog) =
                    executeTransaction(net, *m_envInfo, m_statePre, tr.transaction);
            tr.netId = net;
            transactionResults.push_back(tr);
        }
    }

    if (Options::get().fillchain && _isFilling)
        makeBlockchainTestFromStateTest(networks);

    m_transactions = transactionResults;  // update transactions with execution results.
    return bytes();
}

void ImportTest::checkBalance(eth::State const& _pre, eth::State const& _post, bigint _miningReward)
{
    bigint preBalance = 0;
    bigint postBalance = 0;
    for (auto const& addr : _pre.addresses())
        preBalance += addr.second;
    for (auto const& addr : _post.addresses())
        postBalance += addr.second;

    //account could destroy ether if it suicides to itself
    BOOST_REQUIRE_MESSAGE(preBalance + _miningReward >= postBalance, "Error when comparing states: preBalance + miningReward < postBalance (" + toString(preBalance) + " < " + toString(postBalance) + ") " + TestOutputHelper::get().testName());
}

namespace
{
std::string dumpStackAndMemory(LegacyVM const& _vm)
{
    ostringstream o;
    o << "\n    STACK\n";
    for (auto i : _vm.stack())
        o << (h256)i << "\n";
    o << "    MEMORY\n"
      << ((_vm.memory().size() > 1000) ? " mem size greater than 1000 bytes " :
          memDump(_vm.memory()));
    return o.str();
};

std::string dumpStorage(ExtVM const& _ext)
{
    ostringstream o;
    o << "    STORAGE\n";
    for (auto const& i : _ext.state().storage(_ext.myAddress))
        o << showbase << hex << i.second.first << ": " << i.second.second << "\n";
    return o.str();
};

OnOpFunc simpleTrace()
{
    static auto traceLogger = Logger{createLogger(VerbosityTrace, "vmtrace")};

    return [](uint64_t steps, uint64_t PC, Instruction inst, bigint newMemSize,
               bigint gasCost, bigint gas, VMFace const* _vm, ExtVMFace const* voidExt) {
        ExtVM const& ext = *static_cast<ExtVM const*>(voidExt);
        auto vm = dynamic_cast<LegacyVM const*>(_vm);

        ostringstream o;
        if (vm)
            LOG(traceLogger) << dumpStackAndMemory(*vm);
        LOG(traceLogger) << dumpStorage(ext);
        LOG(traceLogger) << " < " << dec << ext.depth << " : " << ext.myAddress << " : #" << steps
                         << " : " << hex << setw(4) << setfill('0') << PC << " : "
                         << instructionInfo(inst).name << " : " << dec << gas << " : -" << dec
                         << gasCost << " : " << newMemSize << "x32"
                         << " >";
    };
}
}  // namespace

std::tuple<eth::State, ImportTest::ExecOutput, eth::ChangeLog> ImportTest::executeTransaction(eth::Network const _sealEngineNetwork, eth::EnvInfo const& _env, eth::State const& _preState, eth::Transaction const& _tr)
{
    assert(m_envInfo);

    bool removeEmptyAccounts = false;
    State initialState = _preState;
    initialState.addBalance(_env.author(), 0);  // imitate mining reward
    ExecOutput out(std::make_pair(eth::ExecutionResult(), eth::TransactionReceipt(h256(), u256(), eth::LogEntries())));
    try
    {
        unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(_sealEngineNetwork)).createSealEngine());
        removeEmptyAccounts = m_envInfo->number() >= se->chainParams().EIP158ForkBlock;
        if (Options::get().jsontrace)
        {
            StandardTrace st;
            st.setShowMnemonics();
            st.setOptions(Options::get().jsontraceOptions);
            out = initialState.execute(_env, *se.get(), _tr, Permanence::Committed, st.onOp());
            cout << st.multilineTrace();
            cout << "{\"stateRoot\": \"" << initialState.rootHash().hex() << "\"}";
        }
        else
        {
            auto const vmtrace = Options::get().vmtrace ? simpleTrace() : nullptr;
            out = initialState.execute(_env, *se.get(), _tr, Permanence::Uncommitted, vmtrace);
        }

        // the changeLog might be broken under --jsontrace, because it uses intialState.execute with Permanence::Committed rather than Permanence::Uncommitted
        eth::ChangeLog changeLog = initialState.changeLog();
        ImportTest::checkBalance(_preState, initialState);

        //Finalize the state manually (clear logs)
        initialState.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);

        if (!removeEmptyAccounts)
        {
            // Touch here bacuse coinbase might be suicided above
            initialState.addBalance(_env.author(), 0);  // imitate mining reward
            initialState.commit(State::CommitBehaviour::KeepEmptyAccounts);
        }
        return std::make_tuple(initialState, out, changeLog);
    }
    catch (Exception const& _e)
    {
        cnote << "Exception: " << diagnostic_information(_e);
    }
    catch (std::exception const& _e)
    {
        cnote << "state execution exception: " << _e.what();
    }

    initialState.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts :
                                              State::CommitBehaviour::KeepEmptyAccounts);
    return std::make_tuple(initialState, out, initialState.changeLog());
}

json_spirit::mObject ImportTest::makeAllFieldsHex(json_spirit::mObject const& _input, bool _isHeader)
{
    static const set<string> hashes {"bloom" , "coinbase", "hash", "mixHash", "parentHash", "receiptTrie",
                                     "stateRoot", "transactionsTrie", "uncleHash", "currentCoinbase",
                                     "previousHash", "to", "address", "caller", "origin", "secretKey", "data", "extraData"};

    json_spirit::mObject output = _input;

    for (auto const& i: output)
    {
        std::string key = i.first;
        if (key == "extraData")
            continue;

        bool isHash = hashes.count(key);
        bool isData = (key == "data");

        if (_isHeader && key == "nonce")
            isHash = true;

        std::string str;
        json_spirit::mValue value = i.second;

        if (value.type() == json_spirit::int_type)
            str = toString(value.get_int());
        else if (value.type() == json_spirit::str_type)
            str = isData ? replaceCode(value.get_str()) : value.get_str();
        else if (value.type() == json_spirit::array_type)
        {
            json_spirit::mArray arr;
            for (auto const& j: value.get_array())
            {
                str = isData ? replaceCode(j.get_str()) : j.get_str();
                arr.push_back(
                    (str.substr(0, 2) == "0x") ? str : toCompactHexPrefixed(toU256(str), 1));
            }
            output[key] = arr;
            continue;
        }
        else continue;

        if (isHash)
            output[key] = (str.substr(0, 2) == "0x" || str.empty()) ? str : "0x" + str;
        else
            output[key] = (str.substr(0, 2) == "0x") ? str : toCompactHexPrefixed(toU256(str), 1);
    }
    return output;
}

void ImportTest::importEnv(json_spirit::mObject const& _o)
{
    requireJsonFields(_o, "env",
    {{"currentCoinbase", jsonVType::str_type}, {"currentDifficulty", jsonVType::str_type},
     {"currentGasLimit", jsonVType::str_type}, {"currentNumber", jsonVType::str_type},
     {"currentTimestamp", jsonVType::str_type}, {"previousHash", jsonVType::str_type}});
    auto gasLimit = toU256(_o.at("currentGasLimit"));
    BOOST_REQUIRE(gasLimit <= std::numeric_limits<int64_t>::max());
    BlockHeader header;
    header.setGasLimit(gasLimit.convert_to<int64_t>());
    header.setDifficulty(toU256(_o.at("currentDifficulty")));
    header.setNumber(toUint64(_o.at("currentNumber")));
    header.setTimestamp(toUint64(_o.at("currentTimestamp")));
    header.setAuthor(Address(_o.at("currentCoinbase").get_str()));

    m_lastBlockHashes.reset(new TestLastBlockHashes(lastHashes(header.number())));
    m_envInfo.reset(new EnvInfo(header, *m_lastBlockHashes, 0));
}

// import state from not fully declared json_spirit::mObject, writing to _stateOptionsMap which fields were defined in json
void ImportTest::importState(json_spirit::mObject const& _o, State& _state, AccountMaskMap& o_mask)
{
    json_spirit::mObject o = _o;
    replaceCodeInState(
                o);  // Compile LLL and other src code of the test Fillers using external call to lllc
    for (auto const& account : o)
    {
        auto const& accountMaskJson = account.second.get_obj();
        validation::validateAccountMaskObj(accountMaskJson);
    }
    std::string jsondata = json_spirit::write_string((json_spirit::mValue)o, false);
    _state.populateFrom(jsonToAccountMap(jsondata, 0, &o_mask));
}

void ImportTest::importState(json_spirit::mObject const& _o, State& _state)
{
    for (auto const& account : _o)
    {
        BOOST_REQUIRE_MESSAGE(account.second.type() == jsonVType::obj_type,
                              "State account is required to be json Object!");
        requireJsonFields(account.second.get_obj(), account.first,
        {{"balance", jsonVType::str_type}, {"code", jsonVType::str_type},
         {"nonce", jsonVType::str_type}, {"storage", jsonVType::obj_type}});
    }

    AccountMaskMap mask;
    importState(_o, _state, mask);
}

void ImportTest::importTransaction (json_spirit::mObject const& _o, eth::Transaction& o_tr)
{
    if (_o.count("secretKey") > 0)
    {
        requireJsonFields(_o, "transaction",
        {{"data", jsonVType::str_type}, {"gasLimit", jsonVType::str_type},
         {"gasPrice", jsonVType::str_type}, {"nonce", jsonVType::str_type},
         {"secretKey", jsonVType::str_type}, {"to", jsonVType::str_type},
         {"value", jsonVType::str_type}});

        if (bigint(_o.at("nonce").get_str()) >= c_max256plus1)
            BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'nonce' is equal or greater than 2**256") );
        if (bigint(_o.at("gasPrice").get_str()) >= c_max256plus1)
            BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'gasPrice' is equal or greater than 2**256") );
        if (bigint(_o.at("gasLimit").get_str()) >= c_max256plus1)
            BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'gasLimit' is equal or greater than 2**256") );
        if (bigint(_o.at("value").get_str()) >= c_max256plus1)
            BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'value' is equal or greater than 2**256") );

        o_tr = _o.at("to").get_str().empty() ?
                   Transaction(toU256(_o.at("value")), toU256(_o.at("gasPrice")),
                       toU256(_o.at("gasLimit")), importData(_o), toU256(_o.at("nonce")),
                       Secret(_o.at("secretKey").get_str())) :
                   Transaction(toU256(_o.at("value")), toU256(_o.at("gasPrice")),
                       toU256(_o.at("gasLimit")), Address(_o.at("to").get_str()), importData(_o),
                       toU256(_o.at("nonce")), Secret(_o.at("secretKey").get_str()));
    }
    else
    {
        requireJsonFields(_o, "transaction",
        {{"data", jsonVType::str_type}, {"gasLimit", jsonVType::str_type},
         {"gasPrice", jsonVType::str_type}, {"nonce", jsonVType::str_type},
         {"v", jsonVType::str_type}, {"r", jsonVType::str_type}, {"s", jsonVType::str_type},
         {"to", jsonVType::str_type}, {"value", jsonVType::str_type}});

        RLPStream transactionRLPStream = createRLPStreamFromTransactionFields(_o);
        RLP transactionRLP(transactionRLPStream.out());
        try
        {
            o_tr = Transaction(transactionRLP.data(), CheckTransaction::Everything);
        }
        catch (InvalidSignature const&)
        {
            // create unsigned transaction
            o_tr = _o.at("to").get_str().empty() ?
                       Transaction(toU256(_o.at("value")), toU256(_o.at("gasPrice")),
                           toU256(_o.at("gasLimit")), importData(_o), toU256(_o.at("nonce"))) :
                       Transaction(toU256(_o.at("value")), toU256(_o.at("gasPrice")),
                           toU256(_o.at("gasLimit")), Address(_o.at("to").get_str()),
                           importData(_o), toU256(_o.at("nonce")));
        }
        catch (Exception& _e)
        {
            cnote << "invalid transaction" << boost::diagnostic_information(_e);
        }
    }
}

void ImportTest::importTransaction(json_spirit::mObject const& o_tr)
{
    if (o_tr.count("secretKey"))
        requireJsonFields(o_tr, "transaction",
        {{"data", jsonVType::array_type}, {"gasLimit", jsonVType::array_type},
         {"gasPrice", jsonVType::str_type}, {"nonce", jsonVType::str_type},
         {"secretKey", jsonVType::str_type}, {"to", jsonVType::str_type},
         {"value", jsonVType::array_type}});
    else
        requireJsonFields(o_tr, "transaction",
        {{"data", jsonVType::array_type}, {"gasLimit", jsonVType::array_type},
         {"gasPrice", jsonVType::str_type}, {"nonce", jsonVType::str_type},
         {"v", jsonVType::str_type}, {"r", jsonVType::str_type}, {"s", jsonVType::str_type},
         {"to", jsonVType::str_type}, {"value", jsonVType::array_type}});

    // Parse extended transaction
    size_t dataVectorSize = o_tr.at("data").get_array().size();
    size_t gasVectorSize = o_tr.at("gasLimit").get_array().size();
    size_t valueVectorSize = o_tr.at("value").get_array().size();

    BOOST_REQUIRE_MESSAGE(dataVectorSize > 0, "Transaction should has at least 1 data item!");
    BOOST_REQUIRE_MESSAGE(gasVectorSize > 0, "Transaction should has at least 1 gas item!");
    BOOST_REQUIRE_MESSAGE(valueVectorSize > 0, "Transaction should has at least 1 value item!");

    for (size_t d = 0; d < dataVectorSize; d++)
        for (size_t g = 0; g < gasVectorSize; g++)
            for (size_t v = 0; v < valueVectorSize; v++)
            {
                json_spirit::mValue gas = o_tr.at("gasLimit").get_array().at(g);
                json_spirit::mValue value = o_tr.at("value").get_array().at(v);
                json_spirit::mValue data = o_tr.at("data").get_array().at(d);

                json_spirit::mObject o_tr_tmp = o_tr;
                o_tr_tmp["data"] = data;
                o_tr_tmp["gasLimit"] = gas;
                o_tr_tmp["value"] = value;

                importTransaction(o_tr_tmp, m_transaction);

                transactionToExecute execData(d, g, v, m_transaction);
                m_transactions.push_back(execData);
            }
}

int ImportTest::compareStates(State const& _stateExpect, State const& _statePost, AccountMaskMap const _expectedStateOptions, WhenError _throw)
{
    bool wasError = false;
#define CHECK(a,b)						\
    {									\
    if (_throw == WhenError::Throw) \
    {								\
    BOOST_CHECK_MESSAGE(a, b);	\
    if (!a)						\
    return 1;				\
}								\
    else							\
    {								\
    BOOST_WARN_MESSAGE(a,b);	\
    if (!a)						\
    wasError = true;		\
}								\
}

    for (auto const& a: _stateExpect.addresses())
    {
        AccountMask addressOptions(true);
        if(_expectedStateOptions.size())
        {
            try
            {
                addressOptions = _expectedStateOptions.at(a.first);
            }
            catch(std::out_of_range const&)
            {
                BOOST_ERROR(TestOutputHelper::get().testName() + " expectedStateOptions map does not match expectedState in checkExpectedState!");
                break;
            }
        }

        if (addressOptions.shouldExist())
        {
            CHECK(_statePost.addressInUse(a.first), TestOutputHelper::get().testName() +  " Compare States: " << a.first << " missing expected address!");
        }
        else
        {
            CHECK(!_statePost.addressInUse(a.first), TestOutputHelper::get().testName() +  " Compare States: " << a.first << " address not expected to exist!");
        }

        if (_statePost.addressInUse(a.first))
        {

            if (addressOptions.hasBalance())
                CHECK((_stateExpect.balance(a.first) == _statePost.balance(a.first)),
                      TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect balance " << _statePost.balance(a.first) << ", expected " << _stateExpect.balance(a.first));

            if (addressOptions.hasNonce())
                CHECK((_stateExpect.getNonce(a.first) == _statePost.getNonce(a.first)),
                      TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect nonce " << _statePost.getNonce(a.first) << ", expected " << _stateExpect.getNonce(a.first));

            if (addressOptions.hasStorage())
            {
                map<h256, pair<u256, u256>> stateStorage = _statePost.storage(a.first);
                for (auto const& s: _stateExpect.storage(a.first))
                    CHECK((stateStorage[s.first] == s.second),
                            TestOutputHelper::get().testName() + " Check State: " << a.first << ": incorrect storage [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(stateStorage[s.first].second) << ", expected [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(s.second.second));

                //Check for unexpected storage values
                map<h256, pair<u256, u256>> expectedStorage = _stateExpect.storage(a.first);
                for (auto const& s: _statePost.storage(a.first))
                    CHECK((expectedStorage[s.first] == s.second),
                            TestOutputHelper::get().testName() + " Check State: " << a.first << ": incorrect storage [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(s.second.second) << ", expected [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(expectedStorage[s.first].second));
            }

            if (addressOptions.hasCode())
                CHECK((_stateExpect.code(a.first) == _statePost.code(a.first)),
                      TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect code '" << toHexPrefixed(_statePost.code(a.first)) << "', expected '" << toHexPrefixed(_stateExpect.code(a.first)) << "'");
        }
    }

    return wasError;
}

void ImportTest::parseJsonStrValueIntoSet(json_spirit::mValue const& _json, set<string>& _out)
{
    if (_json.type() == json_spirit::array_type)
    {
        for (auto const& val: _json.get_array())
            _out.emplace(val.get_str());
    }
    else
        _out.emplace(_json.get_str());
}

void parseJsonIntValueIntoVector(json_spirit::mValue const& _json, vector<int>& _out)
{
    if (_json.type() == json_spirit::array_type)
    {
        for (auto const& val: _json.get_array())
            _out.push_back(val.get_int());
    }
    else
        _out.push_back(_json.get_int());
}

set<string> const& getAllowedNetworks()
{
    static set<string> allowedNetowks;
    if (allowedNetowks.empty())
    {
        allowedNetowks.emplace("ALL");
        for (auto const& net : test::getNetworks())
            allowedNetowks.emplace(test::netIdToString(net));
    }
    return allowedNetowks;
}

void ImportTest::checkAllowedNetwork(string const& _network)
{
    set<string> const& allowedNetowks = getAllowedNetworks();
    if (!allowedNetowks.count(_network))
    {
        // Can't use boost at this point
        std::cerr << TestOutputHelper::get().testName() + " Specified Network not found: "
                  << _network << "\n";
        exit(1);
    }
}

void ImportTest::checkAllowedNetwork(std::set<std::string> const& _networks)
{
    for (auto const& net : _networks)
        ImportTest::checkAllowedNetwork(net);
}

bool ImportTest::checkGeneralTestSection(json_spirit::mObject const& _expects, vector<size_t>& _errorTransactions, string const& _network) const
{
    return checkGeneralTestSectionSearch(_expects, _errorTransactions, _network, NULL);
}

bool ImportTest::checkGeneralTestSectionSearch(json_spirit::mObject const& _expects, vector<size_t>& _errorTransactions, string const& _network, TrExpectSection* _search) const
{
    if (_expects.count("result"))
    {
        requireJsonFields(_expects, "expect",
        {{"indexes", jsonVType::obj_type}, {"network", jsonVType::array_type},
         {"result", jsonVType::obj_type}});
    }
    else
    {
        // Expect section in filled test
        requireJsonFields(_expects, "expect",
        {{"indexes", jsonVType::obj_type}, {"hash", jsonVType::str_type},
         {"logs", jsonVType::str_type}});
    }

    vector<int> d;
    vector<int> g;
    vector<int> v;
    set<string> network;
    if (_network.empty())
        parseJsonStrValueIntoSet(_expects.at("network"), network);
    else
        network.emplace(_network);

    // replace ">=Homestead" with "Homestead, EIP150, ..."
    network = test::translateNetworks(network);
    BOOST_CHECK_MESSAGE(
                network.size() > 0, TestOutputHelper::get().testName() + " Network array not set!");

    if (!Options::get().singleTestNet.empty())
    {
        //skip this check if we execute transactions only on another specified network
        if (!network.count(Options::get().singleTestNet) && !network.count(string{"ALL"}))
            return false;
    }

    if (_expects.count("indexes"))
    {
        BOOST_REQUIRE_MESSAGE(_expects.at("indexes").type() == jsonVType::obj_type,
                              "indexes field expected to be json Object!");
        json_spirit::mObject const& indexes = _expects.at("indexes").get_obj();
        parseJsonIntValueIntoVector(indexes.at("data"), d);
        parseJsonIntValueIntoVector(indexes.at("gas"), g);
        parseJsonIntValueIntoVector(indexes.at("value"), v);
        BOOST_CHECK_MESSAGE(d.size() > 0 && g.size() > 0 && v.size() > 0, TestOutputHelper::get().testName() + " Indexes arrays not set!");

        //Skip this check if does not fit to options request
        Options const& opt = Options::get();
        if (!inArray(d, opt.trDataIndex) && !inArray(d, -1) && opt.trDataIndex != -1)
            return false;
        if (!inArray(g, opt.trGasIndex) && !inArray(g, -1) && opt.trGasIndex != -1)
            return false;
        if (!inArray(v, opt.trValueIndex) && !inArray(v, -1) && opt.trValueIndex != -1)
            return false;
    }
    else
        BOOST_ERROR(TestOutputHelper::get().testName() + " indexes section not set!");

    bool foundResults = false;
    std::vector<transactionToExecute> lookTransactions;
    if (_search)
        lookTransactions.push_back(_search->first);
    else
        lookTransactions = m_transactions;
    for(size_t i = 0; i < lookTransactions.size(); i++)
    {
        transactionToExecute const& tr = lookTransactions[i];
        if (network.count(netIdToString(tr.netId)) || network.count("ALL"))
            if ((inArray(d, tr.dataInd) || d[0] == -1) && (inArray(g, tr.gasInd) || g[0] == -1) &&
                    (inArray(v, tr.valInd) || v[0] == -1))
            {
                string trInfo = netIdToString(tr.netId) + " data: " + toString(tr.dataInd) +
                        " gas: " + toString(tr.gasInd) + " val: " + toString(tr.valInd);
                if (_expects.count("result"))
                {
                    Options const& opt = Options::get();
                    // filter transactions if a specific index set in options
                    if ((opt.trDataIndex != -1 && opt.trDataIndex != tr.dataInd) ||
                            (opt.trGasIndex != -1 && opt.trGasIndex != tr.gasInd) ||
                            (opt.trValueIndex != -1 && opt.trValueIndex != tr.valInd))
                        continue;

                    State postState = tr.postState;
                    eth::AccountMaskMap stateMap;
                    State expectState(0, OverlayDB(), eth::BaseState::Empty);
                    importState(_expects.at("result").get_obj(), expectState, stateMap);
                    if (_search)
                    {
                        _search->second.first = expectState;
                        _search->second.second = stateMap;
                        return true;
                    }
                    int errcode = compareStates(expectState, postState, stateMap, WhenError::Throw);
                    if (errcode > 0)
                    {
                        cerr << trInfo << "\n";
                        _errorTransactions.push_back(i);
                    }
                }
                else if (_expects.count("hash"))
                {
                    // checking filled state test against client
                    BOOST_CHECK_MESSAGE(_expects.at("hash").get_str() ==
                                        toHexPrefixed(tr.postState.rootHash().asBytes()),
                                        TestOutputHelper::get().testName() + " on " +
                                        test::netIdToString(tr.netId) +
                                        ": Expected another postState hash! expected: " +
                                        _expects.at("hash").get_str() + " actual: " +
                                        toHexPrefixed(tr.postState.rootHash().asBytes()) + " in " + trInfo);
                    if (_expects.count("logs"))
                        BOOST_CHECK_MESSAGE(
                                    _expects.at("logs").get_str() == exportLog(tr.output.second.log()),
                                    TestOutputHelper::get().testName() + " on " +
                                    test::netIdToString(tr.netId) +
                                    " Transaction log mismatch! expected: " +
                                    _expects.at("logs").get_str() +
                                    " actual: " + exportLog(tr.output.second.log()) + " in " + trInfo);
                    else
                        BOOST_ERROR(
                                    TestOutputHelper::get().testName() + "PostState missing logs field!");
                }
                else
                    BOOST_ERROR(TestOutputHelper::get().testName() +
                                " Expect section or postState missing some fields!");

                foundResults = true;

                // if a single transaction check then stop once found
                if (!network.count("ALL") && d[0] != -1 && g[0] != -1 && v[0] != -1)
                    if (network.size() == 1 && d.size() == 1 && g.size() == 1 && v.size() == 1)
                        break;
            }
    }
    if (!_search) //if search for a single transaction in one of the expect sections then don't need this output.
        BOOST_CHECK_MESSAGE(foundResults, TestOutputHelper::get().testName() + " Expect results was not found in test execution!");
    return foundResults;
}

void ImportTest::traceStateDiff()
{
    string network = "ALL";
    Options const& opt = Options::get();
    if (!opt.singleTestNet.empty())
        network = opt.singleTestNet;

    int d = opt.trDataIndex;
    int g = opt.trGasIndex;
    int v = opt.trValueIndex;

    for(auto const& tr : m_transactions)
    {
        if (network == netIdToString(tr.netId) || network == "ALL")
            if ((d == tr.dataInd || d == -1) && (g == tr.gasInd || g == -1) && (v == tr.valInd || v == -1))
            {
                std::ostringstream log;
                log << "trNetID: " << netIdToString(tr.netId) << "\n";
                log << "trDataInd: " << tr.dataInd << " tdGasInd: " << tr.gasInd << " trValInd: " << tr.valInd << "\n";
                LOG(m_logger) << log.str();
                fillJsonWithStateChange(m_statePre, tr.postState, tr.changeLog); //output std log
            }
    }
}

int ImportTest::exportTest()
{
    int err = 0;
    vector<size_t> stateIndexesToPrint;
    if (m_testInputObject.count("expect") > 0)
    {
        BOOST_REQUIRE_MESSAGE(m_testInputObject.at("expect").type() == jsonVType::array_type,
                              "expect section is required to be json Array!");
        for (auto const& exp : m_testInputObject.at("expect").get_array())
        {
            BOOST_REQUIRE_MESSAGE(exp.type() == jsonVType::obj_type,
                                  "expect section element is required to be json Object!");
            checkGeneralTestSection(exp.get_obj(), stateIndexesToPrint);
        }
    }

    std::map<string, json_spirit::mArray> postState;
    for(size_t i = 0; i < m_transactions.size(); i++)
    {
        transactionToExecute const& tr = m_transactions[i];
        json_spirit::mObject obj;
        json_spirit::mObject obj2;
        obj["data"] = tr.dataInd;
        obj["gas"] = tr.gasInd;
        obj["value"] = tr.valInd;
        obj2["indexes"] = obj;
        obj2["hash"] = toHexPrefixed(tr.postState.rootHash().asBytes());
        obj2["logs"] = exportLog(tr.output.second.log());

        //Print the post state if transaction has failed on expect section
        auto it = std::find(std::begin(stateIndexesToPrint), std::end(stateIndexesToPrint), i);
        if (it != std::end(stateIndexesToPrint))
            obj2["postState"] = fillJsonWithState(tr.postState);

        if (Options::get().statediff)
            obj2["stateDiff"] = fillJsonWithStateChange(m_statePre, tr.postState, tr.changeLog);

        postState[netIdToString(tr.netId)].push_back(obj2);
    }

    json_spirit::mObject obj;
    for(std::map<string, json_spirit::mArray>::iterator it = postState.begin(); it != postState.end(); ++it)
        obj[it->first] = it->second;

    m_testOutputObject["post"] = obj;
    m_testOutputObject["pre"] = fillJsonWithState(m_statePre);
    m_testOutputObject["env"] = makeAllFieldsHex(m_testInputObject.at("env").get_obj());
    m_testOutputObject["transaction"] = makeAllFieldsHex(m_testInputObject.at("transaction").get_obj());
    return err;
}
