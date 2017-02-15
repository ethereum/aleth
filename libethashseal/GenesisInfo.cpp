
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
#include "genesis/mainNetworkTest.cpp"
#include "genesis/frontierTest.cpp"
#include "genesis/homesteadTest.cpp"
#include "genesis/eip150Test.cpp"
#include "genesis/eip158Test.cpp"
#include "genesis/metropolisTest.cpp"
#include "genesis/transitionnetTest.cpp"

std::string const& dev::eth::genesisInfo(Network _n)
{
	switch (_n)
	{
	case Network::MainNetwork: return c_genesisInfoMainNetwork;
	case Network::MainNetworkTest: return c_genesisInfoMainNetworkTest;
	case Network::Ropsten: return c_genesisInfoRopsten;
	case Network::TransitionnetTest: return c_genesisInfoTest;
	case Network::FrontierTest: return c_genesisInfoFrontierTest;
	case Network::HomesteadTest: return c_genesisInfoHomesteadTest;
	case Network::EIP150Test: return c_genesisInfoEIP150Test;
	case Network::EIP158Test: return c_genesisInfoEIP158Test;
	case Network::MetropolisTest: return c_genesisInfoMetropolisTest;
	default:
		throw std::invalid_argument("Invalid network value");
	}
}

h256 const& dev::eth::genesisStateRoot(Network _n)
{
	switch (_n)
	{
	case Network::MainNetwork: return c_genesisStateRootMainNetwork;
	case Network::Ropsten: return c_genesisStateRootRopsten;
	case Network::MainNetworkTest: return c_genesisStateRootMainNetworkTest;
	case Network::TransitionnetTest: return c_genesisStateRootTest;
	case Network::FrontierTest: return c_genesisStateRootFrontierTest;
	case Network::HomesteadTest: return c_genesisStateRootHomesteadTest;
	case Network::EIP150Test: return c_genesisStateRootEIP150Test;
	case Network::EIP158Test: return c_genesisStateRootEIP158Test;
	case Network::MetropolisTest: return c_genesisStateRootMetropolisTest;
	default:
		throw std::invalid_argument("Invalid network value");
	}
}
