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
/** @file vm.h
 * @author Christoph Jentzsch <jentzsch.simulationsoftware@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * vm test functions.
 */

#pragma once

#include <test/tools/libtesteth/TestHelper.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libevm/ExtVMFace.h>
#include <libevm/VM.h>
#include <libethereum/Transaction.h>
#include <libethereum/ExtVM.h>
#include <libethereum/State.h>

#include <json_spirit/json_spirit.h>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <cstdint>

namespace dev
{
namespace eth
{
class LastBlockHashesFace;
}

namespace test
{

class FakeExtVM: public eth::ExtVMFace
{
public:
	FakeExtVM() = delete;
	FakeExtVM(eth::EnvInfo const& _envInfo, unsigned _depth = 0);

	virtual u256 store(u256 _n) override { return std::get<2>(addresses[myAddress])[_n]; }
	virtual void setStore(u256 _n, u256 _v) override { std::get<2>(addresses[myAddress])[_n] = _v; }
	virtual bool exists(Address _a) override { return !!addresses.count(_a); }
	virtual u256 balance(Address _a) override { return std::get<0>(addresses[_a]); }
	virtual void suicide(Address _a) override { std::get<0>(addresses[_a]) += std::get<0>(addresses[myAddress]); addresses.erase(myAddress); }
	virtual bytes const& codeAt(Address _a) override { return std::get<3>(addresses[_a]); }
	virtual size_t codeSizeAt(Address _a) override { return std::get<3>(addresses[_a]).size(); }
	virtual std::pair<h160, eth::owning_bytes_ref> create(u256 _endowment, u256& io_gas, bytesConstRef _init, eth::Instruction _op, u256 _salt, eth::OnOpFunc const&) override;
	virtual std::pair<bool, eth::owning_bytes_ref> call(eth::CallParameters&) override;
	virtual h256 blockHash(u256 _number) override;
	void setTransaction(Address _caller, u256 _value, u256 _gasPrice, bytes const& _data);
	void setContract(Address _myAddress, u256 _myBalance, u256 _myNonce, std::map<u256, u256> const& _storage, bytes const& _code);
	void set(Address _a, u256 _myBalance, u256 _myNonce, std::map<u256, u256> const& _storage, bytes const& _code);
	void reset(u256 _myBalance, u256 _myNonce, std::map<u256, u256> const& _storage);
	u256 doPosts();
	json_spirit::mObject exportEnv();
	static dev::eth::EnvInfo importEnv(json_spirit::mObject const& _o, eth::LastBlockHashesFace const& _lastBlockHashes);
	json_spirit::mObject exportState();
	void importState(json_spirit::mObject const& _object);
	json_spirit::mObject exportExec();
	void importExec(json_spirit::mObject const& _o);
	json_spirit::mArray exportCallCreates();
	void importCallCreates(json_spirit::mArray const& _callcreates);

	eth::OnOpFunc simpleTrace() const;

	std::map<Address, std::tuple<u256, u256, std::map<u256, u256>, bytes>> addresses;
	eth::Transactions callcreates;
	bytes thisTxData;
	bytes thisTxCode;
	u256 gas;
	u256 execGas;
};

class VmTestSuite: public TestSuite
{
	json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
	boost::filesystem::path suiteFolder() const override;
	boost::filesystem::path suiteFillerFolder() const override;
};

} } // Namespace Close
