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
/** @file WhisperRPC.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

namespace dev
{
namespace shh
{

class WhisperRPC
{
public:
	WhisperRPC() {}
};

class WhisperRPCClient: public Interface
{
public:
	virtual void inject(shh::Message const& _m, shh::WhisperPeer* _from = nullptr) {}
	
	virtual unsigned installWatch(eth::MessageFilter const& _filter) {};
	virtual unsigned installWatch(h256 _filterId) {};
	virtual void uninstallWatch(unsigned _watchId) {};
	virtual h256s peekWatch(unsigned _watchId) const {};
	virtual h256s checkWatch(unsigned _watchId) {};
	
	virtual shh::Message message(h256 _m) const {};
	
	virtual void sendRaw(bytes const& _payload, bytes const& _topic, unsigned _ttl) {};
	
	WhisperRPCClient() {}
};
	
}
}
