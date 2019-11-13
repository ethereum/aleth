// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcore/Common.h>
#include <libethcore/Common.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/BlockHeader.h>
#include "Account.h"

namespace dev
{
namespace eth
{

class SealEngineFace;

struct ChainParams: public ChainOperationParams
{
    ChainParams();
    ChainParams(ChainParams const& /*_org*/) = default;
    ChainParams(std::string const& _configJson, h256 const& _stateRoot = {},
        boost::filesystem::path const& _configPath = {});
    /// params with additional EIPs activated on top of the last fork block
    ChainParams(std::string const& _configJson, AdditionalEIPs const& _additionalEIPs);
    ChainParams(bytes const& _genesisRLP, AccountMap const& _state)
    {
        populateFromGenesis(_genesisRLP, _state);
    }
    ChainParams(std::string const& _json, bytes const& _genesisRLP, AccountMap const& _state)
      : ChainParams(_json)
    {
        populateFromGenesis(_genesisRLP, _state);
    }

    SealEngineFace* createSealEngine();

    /// Genesis params.
    h256 parentHash = h256();
    Address author = Address();
    u256 difficulty = 1;
    u256 gasLimit = 1 << 31;
    u256 gasUsed = 0;
    u256 timestamp = 0;
    bytes extraData;
    mutable h256 stateRoot;	///< Only pre-populate if known equivalent to genesisState's root. If they're different Bad Things Will Happen.
    AccountMap genesisState;

    unsigned sealFields = 0;
    bytes sealRLP;

    h256 calculateStateRoot(bool _force = false) const;

    /// Genesis block info.
    bytes genesisBlock() const;

private:
    /// load config
    void loadConfig(std::string const& _json, h256 const& _stateRoot,
        boost::filesystem::path const& _configPath);

    void populateFromGenesis(bytes const& _genesisRLP, AccountMap const& _state);

    /// load genesis
    void loadGenesis(std::string const& _json, h256 const& _stateRoot);
};

}
}
