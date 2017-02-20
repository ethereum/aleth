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
/** @file ExtVM.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <map>
#include <functional>
#include <libethcore/Common.h>
#include <libevm/ExtVMFace.h>
#include <libethcore/SealEngine.h>
#include "State.h"
#include "Executive.h"

namespace dev
{
namespace eth
{

class SealEngineFace;

/**
 * @brief Externality interface for the Virtual Machine providing access to world state.
 */
class ExtVM: public ExtVMFace
{
public:
	/// Full constructor.
	ExtVM(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytesConstRef _code, h256 const& _codeHash, unsigned _depth = 0):
		ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(), _codeHash, _depth), m_s(_s), m_sealEngine(_sealEngine)
	{
		// Contract: processing account must exist. In case of CALL, the ExtVM
		// is created only if an account has code (so exist). In case of CREATE
		// the account must be created first.
		assert(m_s.addressInUse(_myAddress));
	}

	/// Read storage location.
	virtual u256 store(u256 _n) override final { return m_s.storage(myAddress, _n); }

	/// Write a value in storage.
	virtual void setStore(u256 _n, u256 _v) override final;

	/// Read address's code.
	virtual bytes const& codeAt(Address _a) override final { return m_s.code(_a); }

	/// @returns the size of the code in  bytes at the given address.
	virtual size_t codeSizeAt(Address _a) override final;

	/// Create a new contract.
	virtual h160 create(u256 _endowment, u256& io_gas, bytesConstRef _code, OnOpFunc const& _onOp = {}) override final;

	/// Create a new message call. Leave _myAddressOverride as the default to use the present address as caller.
	virtual boost::optional<owning_bytes_ref> call(CallParameters& _params) override final;

	/// Read address's balance.
	virtual u256 balance(Address _a) override final { return m_s.balance(_a); }

	/// Does the account exist?
	virtual bool exists(Address _a) override final
	{
		if (evmSchedule().emptinessIsNonexistence())
			return m_s.accountNonemptyAndExisting(_a);
		else
			return m_s.addressInUse(_a);
	}

	/// Suicide the associated contract to the given address.
	virtual void suicide(Address _a) override final;

	/// Return the EVM gas-price schedule for this execution context.
	virtual EVMSchedule const& evmSchedule() const override final { return m_sealEngine.evmSchedule(envInfo()); }

	State const& state() const { return m_s; }

private:
	State& m_s;  ///< A reference to the base state.
	SealEngineFace const& m_sealEngine;
};

}
}

