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
/** @file EthereumFace.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <vector>
#include <map>
#include <libp2p/Common.h>
#include <libdevcore/Common.h>
#include <libethereum/MessageFilter.h>

namespace dev
{
namespace eth
{

class EthereumFace
{
public:
	virtual void flushTransactions() =0;
	virtual std::vector<PeerInfo> peers();
	virtual size_t peerCount();
	virtual void connect(std::string const& _seedHost, unsigned short _port);
	virtual void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice);
	virtual bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice);
	virtual Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice);
	virtual void inject(bytesConstRef _rlp);
	virtual u256 balanceAt(Address _a, int _block) const;
	virtual PastMessages messages(MessageFilter const& _filter) const;
	virtual std::map<u256, u256> storageAt(Address _a, int _block) const;
	virtual u256 countAt(Address _a, int _block) const;
	virtual u256 stateAt(Address _a, u256 _l, int _block) const;
	virtual bytes codeAt(Address _a, int _block);
}

}
}

