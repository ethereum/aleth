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
#include <libdevcore/CommonData.h>
#include <libethereum/Transaction.h>

#include <test/tools/libtesteth/ImportTest.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace dev
{
namespace test
{
json_spirit::mObject fillJsonWithTransaction(Transaction const& _txn)
{
    json_spirit::mObject txObject;
    txObject["nonce"] = toCompactHexPrefixed(_txn.nonce(), 1);
    txObject["data"] = _txn.data().size() ? toHexPrefixed(_txn.data()) : "";
    txObject["gasLimit"] = toCompactHexPrefixed(_txn.gas(), 1);
    txObject["gasPrice"] = toCompactHexPrefixed(_txn.gasPrice(), 1);
    txObject["r"] = toCompactHexPrefixed(_txn.signature().r, 1);
    txObject["s"] = toCompactHexPrefixed(_txn.signature().s, 1);
    txObject["v"] = toCompactHexPrefixed(_txn.signature().v + 27, 1);
    txObject["to"] = _txn.isCreation() ? "" : toHexPrefixed(_txn.receiveAddress());
    txObject["value"] = toCompactHexPrefixed(_txn.value(), 1);
    txObject = ImportTest::makeAllFieldsHex(txObject);
    return txObject;
}

json_spirit::mObject fillJsonWithStateChange(
    State const& _stateOrig, eth::State const& _statePost, eth::ChangeLog const& _changeLog)
{
    json_spirit::mObject oState;
    if (!_changeLog.size())
        return oState;

    // Sort the vector by address field
    eth::ChangeLog changeLog = _changeLog;
    std::sort(changeLog.begin(), changeLog.end(),
        [](const eth::Change& lhs, const eth::Change& rhs) { return lhs.address < rhs.address; });

    std::ostringstream log;
    json_spirit::mObject o;

    size_t i = 0;
    while (i < changeLog.size())
    {
        json_spirit::mArray agregatedBalance;
        json_spirit::mArray agregatedStorage;
        std::map<string, ostringstream> logInfo;
        Address currentAddress = changeLog.at(i).address;
        u256 tmpBalance = _stateOrig.balance(currentAddress);

        while (i < changeLog.size() && changeLog[i].address == currentAddress)
        {
            // Agregate changes for the same addresses
            string after;
            string before;
            json_spirit::mArray record;
            eth::Change change = changeLog.at(i);
            switch (change.kind)
            {
            case Change::Kind::Code:
                // take the original and final code only
                before = toHexPrefixed(_stateOrig.code(change.address));
                after = toHexPrefixed(_statePost.code(change.address));
                record.push_back(before);
                record.push_back("->");
                record.push_back(after);
                o["code"] = record;
                logInfo["code"] << "'code' : ['" + before + "', '->', '" + after + "']\n";
                break;
            case Change::Kind::Nonce:
                // take the original and final nonce only
                before = toCompactHexPrefixed(_stateOrig.getNonce(change.address), 1);
                after = toCompactHexPrefixed(_statePost.getNonce(change.address), 1);
                record.push_back(before);
                record.push_back("->");
                record.push_back(after);
                o["nonce"] = record;
                logInfo["nonce"] << "'nonce' : ['" + before + "', '->', '" + after + "']\n";
                break;
            case Change::Kind::Balance:
                before = toCompactHexPrefixed(tmpBalance, 1);
                after = toCompactHexPrefixed(change.value, 1);
                record.push_back(before);
                record.push_back("+=");
                record.push_back(after);
                tmpBalance += change.value;
                agregatedBalance.push_back(record);
                logInfo["balance"] << "'balance' : ['" + before + "', '+=', '" + after + "']\n";
                break;
            case Change::Kind::Touch:
                o["hasBeenTouched"] = "true";
                logInfo["hasBeenTouched"] << "'hasBeenTouched' : ['true']\n";
                break;
            case Change::Kind::Storage:
                // take the original and final storage only
                before = toCompactHexPrefixed(change.key, 1) + " : " +
                         toCompactHexPrefixed(_stateOrig.storage(change.address, change.key), 1);
                after = toCompactHexPrefixed(change.key, 1) + " : " +
                        toCompactHexPrefixed(_statePost.storage(change.address, change.key), 1);
                record.push_back(before);
                record.push_back("->");
                record.push_back(after);
                agregatedStorage.push_back(record);
                o["storage"] = agregatedStorage;
                logInfo["storage"] << "'storage' : ['" + before + "', '->', '" + after + "']\n";
                break;
            case Change::Kind::Create:
                o["newlyCreated"] = "true";
                logInfo["newlyCreated"] << "'newlyCreated' : ['true']\n";
                break;
            default:
                o["unknownChange"] = "true";
                logInfo["unknownChange"] << "'unknownChange' : ['true']\n";
                break;
            }
            ++i;
        }

        // summaraize changes
        if (agregatedBalance.size())
        {
            json_spirit::mArray record;
            string before = toCompactHexPrefixed(_stateOrig.balance(currentAddress), 1);
            string after = toCompactHexPrefixed(_statePost.balance(currentAddress), 1);
            record.push_back(before);
            record.push_back("->");
            record.push_back(after);
            agregatedBalance.push_back(record);
            o["balance"] = agregatedBalance;
            logInfo["balance"] << "'balance' : ['" + before + "', '->', '" + after + "']\n";
        }

        // commit changes
        oState[toHexPrefixed(currentAddress)] = o;
        log << "\n" << currentAddress << "\n";
        for (std::map<string, ostringstream>::iterator it = logInfo.begin(); it != logInfo.end();
             it++)
            log << (*it).second.str();
    }

    clog(VerbosityInfo, "state") << log.str();
    return oState;
}

json_spirit::mObject fillJsonWithState(State const& _state)
{
    AccountMaskMap emptyMap;
    return fillJsonWithState(_state, emptyMap);
}

json_spirit::mObject fillJsonWithState(State const& _state, eth::AccountMaskMap const& _map)
{
    bool mapEmpty = (_map.size() == 0);
    json_spirit::mObject oState;
    for (auto const& a : _state.addresses())
    {
        if (_map.size() && _map.find(a.first) == _map.end())
            continue;

        json_spirit::mObject o;
        if (mapEmpty || _map.at(a.first).hasBalance())
            o["balance"] = toCompactHexPrefixed(_state.balance(a.first), 1);
        if (mapEmpty || _map.at(a.first).hasNonce())
            o["nonce"] = toCompactHexPrefixed(_state.getNonce(a.first), 1);

        if (mapEmpty || _map.at(a.first).hasStorage())
        {
            json_spirit::mObject store;
            for (auto const& s : _state.storage(a.first))
                store[toCompactHexPrefixed(s.second.first, 1)] =
                    toCompactHexPrefixed(s.second.second, 1);
            o["storage"] = store;
        }

        if (mapEmpty || _map.at(a.first).hasCode())
        {
            if (_state.code(a.first).size() > 0)
                o["code"] = toHexPrefixed(_state.code(a.first));
            else
                o["code"] = "";
        }
        oState[toHexPrefixed(a.first)] = o;
    }
    return oState;
}

}  // test
}  // dev
