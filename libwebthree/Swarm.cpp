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
/** @file Swarm.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Swarm.h"

#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Hash.h>
#include <libethcore/Common.h>
#include <libethereum/Client.h>
#include "WebThree.h"
#include "Support.h"
#include "IPFS.h"
using namespace std;
using namespace dev;
using namespace bzz;
using namespace eth;

struct SwarmChannel: public LogChannel { static const char* name(); static const int verbosity = 3; };
const char* SwarmChannel::name() { return EthYellow "bzz"; }
#define cbzz clog(SwarmChannel)

bzz::Interface::~Interface()
{
}

bzz::Client::Client(WebThreeDirect* _web3):
	m_ipfs(new IPFS)
{
	m_owner = _web3;
}

Pinneds bzz::Client::insertBundle(bytesConstRef _bundle)
{
	cbzz << "Bundle insert" << sha3(_bundle);

	Pinneds ret;
	RLP rlp(_bundle);
	bool first = true;
	for (auto const& r: rlp)
		if (first)
			first = false;
		else
		{
			cbzz << "   inserting slice" << sha3(r.toBytesConstRef());
			ret.push_back(Pinned(m_cache[sha3(r.toBytesConstRef())] = make_shared<bytes>(r.toBytes())));
		}
	return ret;
}

Pinned bzz::Client::put(bytes const& _data)
{
	h256 ret = sha3(_data);
	cbzz << "Inserting" << ret;

	if (!m_cache.count(ret))
		m_cache[ret] = make_shared<bytes>(_data);

	if (m_putAccount)
	{
		// send to IPFS...
		h256 sha256hash = sha256(&_data);
		cbzz << "IPFS-inserting" << sha256hash;

		// set in blockchain
		try
		{
			m_owner->support()->hintSHA256(ret, sha256hash, m_putAccount);
			m_ipfs->putBlockForSHA256(&_data);
		}
		catch (InterfaceNotSupported&) {}
	}

	return Pinned(m_cache[ret]);
}

Pinned bzz::Client::get(h256 const& _hash)
{
	cbzz << "Looking up" << _hash;
	auto it = m_cache.find(_hash);
	if (it != m_cache.end())
		return it->second;

	if (u256 sha256hash = m_owner->support()->sha256Hint(_hash))
	{
		cbzz << "IPFS Searching" << sha256hash;
		auto b = m_ipfs->getBlockForSHA256(sha256hash);
		if (!b.empty())
			return (m_cache[_hash] = make_shared<bytes>(b));
	}

	cbzz << "Not found" << _hash;
	throw ResourceNotAvailable();
}
