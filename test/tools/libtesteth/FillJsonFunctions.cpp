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
#include <libethereum/Transaction.h>
#include <libdevcore/CommonData.h>

#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/ImportTest.h>

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
	txObject["nonce"] = toCompactHexPrefix(_txn.nonce(), 1);
	txObject["data"] = _txn.data().size() ? toHexPrefix(_txn.data()) : "";
	txObject["gasLimit"] = toCompactHexPrefix(_txn.gas(), 1);
	txObject["gasPrice"] = toCompactHexPrefix(_txn.gasPrice(), 1);
	txObject["r"] = toCompactHexPrefix(_txn.signature().r, 1);
	txObject["s"] = toCompactHexPrefix(_txn.signature().s, 1);
	txObject["v"] = toCompactHexPrefix(_txn.signature().v + 27, 1);
	txObject["to"] = _txn.isCreation() ? "" : "0x" + toString(_txn.receiveAddress());
	txObject["value"] = toCompactHexPrefix(_txn.value(), 1);
	ImportTest::makeAllFieldsHex(txObject);
	return txObject;
}

json_spirit::mObject fillJsonWithStateChange(State const& _stateOrig, eth::State const& _statePost, eth::ChangeLog const& _changeLog)
{
	json_spirit::mObject oState;
	if (!_changeLog.size())
		return oState;

	//Sort the vector by address field
	eth::ChangeLog changeLog = _changeLog;
	std::sort(changeLog.begin(), changeLog.end(),
		[](const eth::Change& lhs, const eth::Change& rhs )
		{
			return lhs.address < rhs.address;
		}
	);

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
			//Agregate changes for the same addresses
			string after;
			string before;
			json_spirit::mArray record;
			eth::Change change = changeLog.at(i);
			switch (change.kind)
			{
				case Change::Kind::Code:
					//take the original and final code only
					before = toHexPrefix(_stateOrig.code(change.address));
					after = toHexPrefix(_statePost.code(change.address));
					record.push_back(before);
					record.push_back("->");
					record.push_back(after);
					o["code"] = record;
					logInfo["code"] << "'code' : ['" + before + "', '->', '" + after + "']" << std::endl;
				break;
				case Change::Kind::Nonce:
					//take the original and final nonce only
					before = toCompactHexPrefix(_stateOrig.getNonce(change.address), 1);
					after = toCompactHexPrefix(_statePost.getNonce(change.address), 1);
					record.push_back(before);
					record.push_back("->");
					record.push_back(after);
					o["nonce"] = record;
					logInfo["nonce"] << "'nonce' : ['" + before + "', '->', '" + after + "']" << std::endl;
				break;
				case Change::Kind::Balance:
					before = toCompactHexPrefix(tmpBalance, 1);
					after = toCompactHexPrefix(change.value, 1);
					record.push_back(before);
					record.push_back("+=");
					record.push_back(after);
					tmpBalance += change.value;
					agregatedBalance.push_back(record);
					logInfo["balance"] << "'balance' : ['" + before + "', '+=', '" + after + "']" << std::endl;
				break;
				case Change::Kind::Touch:
					o["hasBeenTouched"] = "true";
					logInfo["hasBeenTouched"] << "'hasBeenTouched' : ['true']" << std::endl;
				break;
				case Change::Kind::Storage:
					//take the original and final storage only
					before = toCompactHexPrefix(change.key, 1) + " : " + toCompactHexPrefix(_stateOrig.storage(change.address, change.key), 1);
					after = toCompactHexPrefix(change.key, 1) + " : " + toCompactHexPrefix(_statePost.storage(change.address, change.key), 1);
					record.push_back(before);
					record.push_back("->");
					record.push_back(after);
					agregatedStorage.push_back(record);
					o["storage"] = agregatedStorage;
					logInfo["storage"] << "'storage' : ['" + before + "', '->', '" + after + "']" << std::endl;
				break;
				case Change::Kind::Create:
					o["newlyCreated"] = "true";
					logInfo["newlyCreated"] << "'newlyCreated' : ['true']" << std::endl;
				break;
				default:
					o["unknownChange"] = "true";
					logInfo["unknownChange"] << "'unknownChange' : ['true']" << std::endl;
				break;
			}
			++i;
		}

		//summaraize changes
		if (agregatedBalance.size())
		{
			json_spirit::mArray record;
			string before = toCompactHexPrefix(_stateOrig.balance(currentAddress), 1);
			string after = toCompactHexPrefix(_statePost.balance(currentAddress), 1);
			record.push_back(before);
			record.push_back("->");
			record.push_back(after);
			agregatedBalance.push_back(record);
			o["balance"] = agregatedBalance;
			logInfo["balance"] << "'balance' : ['" + before + "', '->', '" + after + "']" << std::endl;
		}

		//commit changes
		oState["0x" + toString(currentAddress)] = o;
		log << endl << currentAddress << endl;
		for (std::map<string, ostringstream>::iterator it = logInfo.begin(); it != logInfo.end(); it++)
			log << (*it).second.str();
	}

	dev::LogOutputStream<eth::StateTrace, false>() << log.str();
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
	for (auto const& a: _state.addresses())
	{
		if (_map.size() && _map.find(a.first) == _map.end())
			continue;

		json_spirit::mObject o;
		if (mapEmpty || _map.at(a.first).hasBalance())
			o["balance"] = toCompactHexPrefix(_state.balance(a.first), 1);
		if (mapEmpty || _map.at(a.first).hasNonce())
			o["nonce"] = toCompactHexPrefix(_state.getNonce(a.first), 1);

		if (mapEmpty || _map.at(a.first).hasStorage())
		{
			json_spirit::mObject store;
			for (auto const& s: _state.storage(a.first))
				store[toCompactHexPrefix(s.second.first, 1)] = toCompactHexPrefix(s.second.second, 1);
			o["storage"] = store;
		}

		if (mapEmpty || _map.at(a.first).hasCode())
		{
			if (_state.code(a.first).size() > 0)
				o["code"] = toHexPrefix(_state.code(a.first));
			else
				o["code"] = "";
		}
		oState["0x" + toString(a.first)] = o;
	}
	return oState;
}

}//test
}//dev
