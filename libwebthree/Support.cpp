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

#include "Support.h"
#include <boost/algorithm/string.hpp>
#include <libdevcore/Log.h>
#include <libethcore/ICAP.h>
#include <libethereum/Client.h>
#include "WebThree.h"
using namespace std;
using namespace dev;
using namespace eth;

Support::Support(WebThreeDirect* _web3):
	m_web3(_web3)
{
}

Support::~Support()
{
}

strings Support::decomposed(std::string const& _name)
{
	size_t i = _name.find_first_of('/');
	strings parts;
	string ts = _name.substr(0, i);
	boost::algorithm::split(parts, ts, boost::algorithm::is_any_of("."));
	std::reverse(parts.begin(), parts.end());
	if (i != string::npos)
	{
		strings pathParts;
		boost::algorithm::split(pathParts, ts = _name.substr(i + 1), boost::is_any_of("/"));
		parts += pathParts;
	}
	return parts;
}

bytes Support::call(Address const& _to, bytes const& _data) const
{
	return m_web3->ethereum()->call(_to, _data).output;
}

bytes Support::auxLookup(strings const& _path, std::string const& _query) const
{
	Address address = registrar();
	for (unsigned i = 0; i < _path.size() - 1; ++i)
		address = abiOut<Address>(call(address, abiIn("subRegistrar(string)", _path[i])));
	return call(address, abiIn(_query + "(string)", _path.back()));
}

Address Support::registrar() const
{
	if (c_network == Network::Frontier)
		return Address("c6d9d2cd449a754c494264e1809c50e34d64562b");
	else
		return Address("5e70c0bbcd5636e0f9f9316e9f8633feb64d4050");
}

Address Support::urlHint() const
{
	return lookup<Address>({"urlhinter"}, "addr");
}

Address Support::sha256Hint() const
{
	return lookup<Address>({"sha256hinter"}, "addr");
}

std::string Support::urlHint(h256 const& _content) const
{
	return toString(abiOut<string32>(call(urlHint(), abiIn("url(bytes32)", _content))));
}

h256 Support::sha256Hint(h256 const& _content) const
{
	return abiOut<h256>(call(sha256Hint(), abiIn("data(bytes32)", _content)));
}

void Support::hintURL(h256 const& _content, std::string const& _url, Secret const& _s) const
{
	TransactionSkeleton t;
	t.data = abiIn("suggestUrl(bytes32,bytes32)", _content, toString32(_url));
	t.to = urlHint();
	m_web3->ethereum()->submitTransaction(t, _s);
}

void Support::hintSHA256(h256 const& _content, h256 const& _sha256, Secret const& _s) const
{
	TransactionSkeleton t;
	t.data = abiIn("setData(bytes32,bytes32)", _content, _sha256);
	t.to = sha256Hint();
	m_web3->ethereum()->submitTransaction(t, _s);
}

Address Support::icapRegistrar() const
{
	return Address("a1a111bc074c9cfa781f0c38e63bd51c91b8af00");
	return lookup<Address>({"icapregistrar"}, "addr");
}

std::pair<Address, bytes> Support::decodeICAP(std::string const& _icap) const
{
	return ICAP::decoded(_icap).address([&](Address a, bytes b){ return call(a, b); }, icapRegistrar());
}
