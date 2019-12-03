// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <string>
#include <libdevcore/FixedHash.h>
#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

/// The network id.
enum class Network
{
    ///< Normal Olympic chain.
    // Olympic = 0,
    /// Normal Frontier/Homestead/DAO/EIP150/EIP158/Byzantium/Constantinople chain.
    MainNetwork = 1,
    /// Normal Morden chain.
    // Morden = 2,
    /// New Ropsten Test Network
    Ropsten = 3,
    /// MainNetwork rules but without genesis accounts (for transaction tests).
    MainNetworkTest = 69,
    /// Normal Frontier/Homestead/DAO/EIP150/EIP158 chain without all the premine.
    TransitionnetTest = 70,
    /// Just test the Frontier-era characteristics "forever" (no Homestead portion).
    FrontierTest = 71,
    /// Just test the Homestead-era characteristics "forever" (no Frontier portion).
    HomesteadTest = 72,
    /// Homestead + EIP150 Rules active from block 0 For BlockchainTests
    EIP150Test = 73,
    /// Homestead + EIP150 + EIP158 Rules active from block 0
    EIP158Test = 74,
    /// EIP158Test + Byzantium active from block 0
    ByzantiumTest = 75,
    /// EIP158Test + Byzantium active from block 2
    ByzantiumTransitionTest = 76,
    /// Frontier rules + NoProof seal engine
    FrontierNoProofTest = 77,
    /// ByzantiumTest + Constantinople active from block 0
    ConstantinopleTest = 78,
    /// ConstantinopleTest + Experimental active from block 2
    ExperimentalTransitionTest = 79,
    /// MainNetwork rules without genesis accounts + NoProof seal engine
    MainNetworkNoProofTest = 80,
    /// Byzantium rules + NoProof seal engine
    ByzantiumNoProofTest = 81,
    /// Constantinople rules + NoProof seal engine
    ConstantinopleNoProofTest = 82,
    /// Byzantium + Constantinople + ConstantinopleFix active from block 0
    ConstantinopleFixTest = 83,
    /// Istanbul active from block 0
    IstanbulTest = 84,
    /// ConstantinopleFixTest + Istanbul active from block 2
    IstanbulTransitionTest = 85,
    /// Muir Glacier active from block 0
    MuirGlacierTest = 86,
    /// Berlin active from block 0
    BerlinTest = 87,


    // TransitionTest networks
    FrontierToHomesteadAt5 = 100,
    HomesteadToDaoAt5 = 101,
    HomesteadToEIP150At5 = 102,
    EIP158ToByzantiumAt5 = 103,
    ByzantiumToConstantinopleFixAt5 = 104,

    Special = 0xff  ///< Something else.
};

std::string const& genesisInfo(Network _n);
h256 const& genesisStateRoot(Network _n);

}
}
