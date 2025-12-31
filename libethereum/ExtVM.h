// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Executive.h"
#include "State.h"

#include <libethcore/Common.h>
#include <libethcore/SealEngine.h>
#include <libevm/ExtVMFace.h>

#include <functional>
#include <map>

namespace dev
{
namespace eth
{

class SealEngineFace;

/// Externality interface for the Virtual Machine providing access to world state.
class ExtVM : public ExtVMFace
{
public:
    /// Full constructor.
    ExtVM(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Address _myAddress,
        Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data,
        bytesConstRef _code, h256 const& _codeHash, u256 const& _version, unsigned _depth,
        bool _isCreate, bool _staticCall)
      : ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(),
            _codeHash, _version, _depth, _isCreate, _staticCall),
        m_s(_s),
        m_sealEngine(_sealEngine),
        m_evmSchedule(initEvmSchedule(envInfo().number(), _version))
    {
        // Contract: processing account must exist. In case of CALL, the ExtVM
        // is created only if an account has code (so exist). In case of CREATE
        // the account must be created first.
        assert(m_s.addressInUse(_myAddress));
    }

    /// Read storage location.
    u256 store(u256 _n) final { return m_s.storage(myAddress, _n); }

    /// Write a value in storage.
    void setStore(u256 _n, u256 _v) final;

    /// Read original storage value (before modifications in the current transaction).
    u256 originalStorageValue(u256 const& _key) final
    {
        return m_s.originalStorageValue(myAddress, _key);
    }

    /// Read address's code.
    bytes const& codeAt(Address _a) final { return m_s.code(_a); }

    /// @returns the size of the code in  bytes at the given address.
    size_t codeSizeAt(Address _a) final;

    /// @returns the hash of the code at the given address.
    h256 codeHashAt(Address _a) final;

    /// Create a new contract.
    CreateResult create(u256 _endowment, u256& io_gas, bytesConstRef _code, Instruction _op, u256 _salt, OnOpFunc const& _onOp = {}) final;

    /// Create a new message call.
    CallResult call(CallParameters& _params) final;

    /// Read address's balance.
    u256 balance(Address _a) final { return m_s.balance(_a); }

    /// Does the account exist?
    bool exists(Address _a) final
    {
        if (evmSchedule().emptinessIsNonexistence())
            return m_s.accountNonemptyAndExisting(_a);
        else
            return m_s.addressInUse(_a);
    }

    /// Selfdestruct the associated contract to the given address.
    void selfdestruct(Address _a) final;

    /// Return the EVM gas-price schedule for this execution context.
    EVMSchedule const& evmSchedule() const final { return m_evmSchedule; }

    State const& state() const { return m_s; }

    /// Hash of a block if within the last 256 blocks, or h256() otherwise.
    h256 blockHash(u256 _number) final;

private:
    EVMSchedule const& initEvmSchedule(int64_t _blockNumber, u256 const& _version) const
    {
        // If _version is latest for the block, select corresponding latest schedule.
        // Otherwise run with the latest schedule known to correspond to the _version.
        EVMSchedule const& currentBlockSchedule = m_sealEngine.evmSchedule(_blockNumber);
        if (currentBlockSchedule.accountVersion == _version)
            return currentBlockSchedule;
        else
            return latestScheduleForAccountVersion(_version);
    }


    State& m_s;  ///< A reference to the base state.
    SealEngineFace const& m_sealEngine;
    EVMSchedule const& m_evmSchedule;
};

}
}

