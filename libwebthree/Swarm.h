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
/** @file Swarm.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <unordered_map>
#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcrypto/Common.h>

namespace dev
{

class WebThreeDirect;
class IPFS;

namespace bzz
{

DEV_SIMPLE_EXCEPTION(ResourceNotAvailable);

/// Pinned, read-only, content.
class Pinned
{
	friend class Client;

public:
	Pinned() = default;

	explicit operator bool() const { return !!m_content; }

	operator bytes const&() const { return *m_content; }
	operator bytesConstRef() const { return bytesConstRef(&*m_content); }

private:
	Pinned(std::shared_ptr<bytes> const& _d): m_content(_d) {}

	std::shared_ptr<bytes> m_content;
};

using Pinneds = std::vector<Pinned>;

/// Basic interface for Swarm.
class Interface
{
public:
	virtual ~Interface();

	virtual Pinned put(bytes const& _data) = 0;
	virtual Pinned get(h256 const& _hash) = 0;

	virtual Pinneds insertBundle(bytesConstRef _bundle) = 0;
};

class Client;

/// Placeholder for Swarm.
class Client: public Interface
{
public:
	Client(WebThreeDirect* _web3);

	void setPutAccount(Secret const& _s) { m_putAccount = _s; }

	Pinned put(bytes const& _data) override;
	Pinned get(h256 const& _hash) override;

	Pinneds insertBundle(bytesConstRef _bundle) override;

private:
	std::unordered_map<h256, std::shared_ptr<bytes>> m_cache;

	WebThreeDirect* m_owner;
	std::shared_ptr<IPFS> m_ipfs;

	Secret m_putAccount;
};

}
}
