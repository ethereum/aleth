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
/** @file Net.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once
#include "NetFace.h"

namespace dev
{

class NetworkFace;

namespace rpc
{

class Net: public NetFace
{
public:
	Net(NetworkFace& _network);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"net", "1.0"}};
	}
	virtual std::string net_version() override;
	virtual std::string net_peerCount() override;
	virtual bool net_listening() override;

private:
	NetworkFace& m_network;
};

}
}
