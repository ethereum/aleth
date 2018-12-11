
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

#include "GenesisInfo.h"
using namespace dev;

//Client configurations
#include "genesis/mainNetwork.cpp"
#include "genesis/ropsten.cpp"

//Test configurations
#include "genesis/test/byzantiumNoProofTest.cpp"
#include "genesis/test/byzantiumTest.cpp"
#include "genesis/test/byzantiumTransitionTest.cpp"
#include "genesis/test/constantinopleNoProofTest.cpp"
#include "genesis/test/constantinopleTest.cpp"
#include "genesis/test/eip150Test.cpp"
#include "genesis/test/eip158Test.cpp"
#include "genesis/test/experimentalTransitionTest.cpp"
#include "genesis/test/frontierNoProofTest.cpp"
#include "genesis/test/frontierTest.cpp"
#include "genesis/test/homesteadTest.cpp"
#include "genesis/test/mainNetworkNoProofTest.cpp"
#include "genesis/test/mainNetworkTest.cpp"

//Transition configurations
#include "genesis/test/ByzantiumToConstantinopleAt5Test.cpp"
#include "genesis/test/EIP158ToByzantiumAt5Test.cpp"
#include "genesis/test/frontierToHomesteadAt5Test.cpp"
#include "genesis/test/homesteadToDaoAt5Test.cpp"
#include "genesis/test/homesteadToEIP150At5Test.cpp"
#include "genesis/test/transitionnetTest.cpp"

static dev::h256 const c_genesisDefaultStateRoot;

std::string const& dev::eth::genesisInfo(Network _n)
{
    switch (_n)
    {
    //Client genesis
    case Network::MainNetwork: return c_genesisInfoMainNetwork;
    case Network::Ropsten: return c_genesisInfoRopsten;

    //Test genesis
    case Network::MainNetworkTest: return c_genesisInfoMainNetworkTest;
    case Network::MainNetworkNoProofTest: return c_genesisInfoMainNetworkNoProofTest;
    case Network::FrontierNoProofTest: return c_genesisInfoFrontierNoProofTest;
    case Network::FrontierTest: return c_genesisInfoFrontierTest;
    case Network::HomesteadTest: return c_genesisInfoHomesteadTest;
    case Network::EIP150Test: return c_genesisInfoEIP150Test;
    case Network::EIP158Test: return c_genesisInfoEIP158Test;
    case Network::ByzantiumTest: return c_genesisInfoByzantiumTest;
    case Network::ByzantiumNoProofTest:
        return c_genesisInfoByzantiumNoProofTest;
    case Network::ByzantiumTransitionTest: return c_genesisInfoByzantiumTransitionTest;
    case Network::ConstantinopleTest:
        return c_genesisInfoConstantinopleTest;
    case Network::ConstantinopleNoProofTest:
        return c_genesisInfoConstantinopleNoProofTest;
    case Network::ExperimentalTransitionTest:
        return c_genesisInfoExperimentalTransitionTest;

    //Transition test genesis
    case Network::FrontierToHomesteadAt5: return c_genesisInfoFrontierToHomesteadAt5Test;
    case Network::HomesteadToDaoAt5: return c_genesisInfoHomesteadToDaoAt5Test;
    case Network::HomesteadToEIP150At5: return c_genesisInfoHomesteadToEIP150At5Test;
    case Network::EIP158ToByzantiumAt5: return c_genesisInfoEIP158ToByzantiumAt5Test;
    case Network::ByzantiumToConstantinopleAt5:
        return c_genesisInfoByzantiumToConstantinopleAt5Test;
    case Network::TransitionnetTest: return c_genesisInfoTest;

    default:
        throw std::invalid_argument("Invalid network value");
    }
}

h256 const& dev::eth::genesisStateRoot(Network _n)
{
    switch (_n)
    {
    case Network::MainNetwork: return c_genesisStateRootMainNetwork;
    case Network::Ropsten:
    case Network::MainNetworkTest:
    case Network::TransitionnetTest:
    case Network::FrontierTest:
    case Network::HomesteadTest: \
    case Network::EIP150Test:
    case Network::EIP158Test:
    case Network::ByzantiumTest:
    case Network::ConstantinopleTest:
    case Network::ExperimentalTransitionTest:
        return c_genesisDefaultStateRoot;
    default:
        throw std::invalid_argument("Invalid network value");
    }
}
