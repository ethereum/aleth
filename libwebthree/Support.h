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
/** @file Support.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <libethcore/Common.h>
#include <libethcore/ABI.h>
#include <libethereum/Interface.h>

namespace dev
{

class WebThreeDirect;

/// Utilities supporting typical use of the Ethereum network.
class Support
{
public:
	Support(WebThreeDirect* _web3);
	~Support();

	// URI managment
	static strings decomposed(std::string const& _name);

	// Registrar
	Address registrar() const;
	h256 content(std::string const& _url) const { return lookup<h256>(decomposed(_url), "content"); }
	Address address(std::string const& _url) const { return lookup<Address>(decomposed(_url), "addr"); }
	Address subRegistrar(std::string const& _url) const { return lookup<Address>(decomposed(_url), "subRegistrar"); }
	Address owner(std::string const& _url) const { return lookup<Address>(decomposed(_url), "owner"); }

	// URL Hinter
	Address urlHint() const;
	std::string urlHint(h256 const& _content) const;
	void hintURL(h256 const& _content, std::string const& _url, Secret const& _s) const;

	// SHA256 Hinter
	Address sha256Hint() const;
	h256 sha256Hint(h256 const& _content) const;
	void hintSHA256(h256 const& _content, h256 const& _sha256, Secret const& _s) const;

	// ICAP
	Address icapRegistrar() const;
	std::pair<Address, bytes> decodeICAP(std::string const& _icap) const;

private:
	bytes call(Address const& _to, bytes const& _data) const;

	template <class T> T lookup(strings const& _path, std::string const& _query) const
	{
		return eth::abiOut<T>(auxLookup(_path, _query));
	}
	bytes auxLookup(strings const& _path, std::string const& _query) const;

	WebThreeDirect* m_web3;
};

}
