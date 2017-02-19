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
/** @file Network.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @author Eric Lombrozo <elombrozo@gmail.com> (Windows version of getInterfaceAddresses())
 * @date 2014
 */

#include <sys/types.h>
#ifndef _WIN32
#include <ifaddrs.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <libdevcore/Common.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Exceptions.h>
#include "Common.h"
#include "UPnP.h"
#include "Network.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;

static_assert(BOOST_VERSION == 106300, "Wrong boost headers version");

std::set<bi::address> Network::getInterfaceAddresses()
{
	std::set<bi::address> addresses;

#if defined(_WIN32)
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		BOOST_THROW_EXCEPTION(NoNetworking());

	char ac[80];
	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
	{
		clog(NetWarn) << "Error " << WSAGetLastError() << " when getting local host name.";
		WSACleanup();
		BOOST_THROW_EXCEPTION(NoNetworking());
	}

	struct hostent* phe = gethostbyname(ac);
	if (phe == 0)
	{
		clog(NetWarn) << "Bad host lookup.";
		WSACleanup();
		BOOST_THROW_EXCEPTION(NoNetworking());
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i)
	{
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		char *addrStr = inet_ntoa(addr);
		bi::address address(bi::address::from_string(addrStr));
		if (!isLocalHostAddress(address))
			addresses.insert(address.to_v4());
	}

	WSACleanup();
#else
	ifaddrs* ifaddr;
	if (getifaddrs(&ifaddr) == -1)
		BOOST_THROW_EXCEPTION(NoNetworking());

	for (auto ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (!ifa->ifa_addr || string(ifa->ifa_name) == "lo0" || !(ifa->ifa_flags & IFF_UP))
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET)
		{
			in_addr addr = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			boost::asio::ip::address_v4 address(boost::asio::detail::socket_ops::network_to_host_long(addr.s_addr));
			if (!isLocalHostAddress(address))
				addresses.insert(address);
		}
		else if (ifa->ifa_addr->sa_family == AF_INET6)
		{
			sockaddr_in6* sockaddr = ((struct sockaddr_in6 *)ifa->ifa_addr);
			in6_addr addr = sockaddr->sin6_addr;
			boost::asio::ip::address_v6::bytes_type bytes;
			memcpy(&bytes[0], addr.s6_addr, 16);
			boost::asio::ip::address_v6 address(bytes, sockaddr->sin6_scope_id);
			if (!isLocalHostAddress(address))
				addresses.insert(address);
		}
	}

	if (ifaddr!=NULL)
		freeifaddrs(ifaddr);

#endif

	return addresses;
}

int Network::tcp4Listen(bi::tcp::acceptor& _acceptor, NetworkPreferences const& _netPrefs)
{
	// Due to the complexities of NAT and network environments (multiple NICs, tunnels, etc)
	// and security concerns automation is the enemy of network configuration.
	// If a preference cannot be accommodate the network must fail to start.
	//
	// Preferred IP: Attempt if set, else, try 0.0.0.0 (all interfaces)
	// Preferred Port: Attempt if set, else, try c_defaultListenPort or 0 (random)
	// TODO: throw instead of returning -1 and rename NetworkPreferences to NetworkConfig
	
	bi::address listenIP;
	try
	{
		listenIP = _netPrefs.listenIPAddress.empty() ? bi::address_v4() : bi::address::from_string(_netPrefs.listenIPAddress);
	}
	catch (...)
	{
		cwarn << "Couldn't start accepting connections on host. Failed to accept socket on " << listenIP << ":" << _netPrefs.listenPort << ".\n" << boost::current_exception_diagnostic_information();
		return -1;
	}
	bool requirePort = (bool)_netPrefs.listenPort;

	for (unsigned i = 0; i < 2; ++i)
	{
		bi::tcp::endpoint endpoint(listenIP, requirePort ? _netPrefs.listenPort : (i ? 0 : c_defaultListenPort));
		try
		{
#if defined(_WIN32)
			bool reuse = false;
#else
			bool reuse = true;
#endif
			_acceptor.open(endpoint.protocol());
			_acceptor.set_option(ba::socket_base::reuse_address(reuse));
			_acceptor.bind(endpoint);
			_acceptor.listen();
			return _acceptor.local_endpoint().port();
		}
		catch (...)
		{
			// bail if this is first attempt && port was specificed, or second attempt failed (random port)
			if (i || requirePort)
			{
				// both attempts failed
				cwarn << "Couldn't start accepting connections on host. Failed to accept socket on " << listenIP << ":" << _netPrefs.listenPort << ".\n" << boost::current_exception_diagnostic_information();
				_acceptor.close();
				return -1;
			}
			
			_acceptor.close();
			continue;
		}
	}

	return -1;
}

bi::tcp::endpoint Network::traverseNAT(std::set<bi::address> const& _ifAddresses, unsigned short _listenPort, bi::address& o_upnpInterfaceAddr)
{
	asserts(_listenPort != 0);

	unique_ptr<UPnP> upnp;
	try
	{
		upnp.reset(new UPnP);
	}
	// let m_upnp continue as null - we handle it properly.
	catch (...) {}

	bi::tcp::endpoint upnpEP;
	if (upnp && upnp->isValid())
	{
		bi::address pAddr;
		int extPort = 0;
		for (auto const& addr: _ifAddresses)
			if (addr.is_v4() && isPrivateAddress(addr) && (extPort = upnp->addRedirect(addr.to_string().c_str(), _listenPort)))
			{
				pAddr = addr;
				break;
			}

		auto eIP = upnp->externalIP();
		bi::address eIPAddr(bi::address::from_string(eIP));
		if (extPort && eIP != string("0.0.0.0") && !isPrivateAddress(eIPAddr))
		{
			clog(NetNote) << "Punched through NAT and mapped local port" << _listenPort << "onto external port" << extPort << ".";
			clog(NetNote) << "External addr:" << eIP;
			o_upnpInterfaceAddr = pAddr;
			upnpEP = bi::tcp::endpoint(eIPAddr, (unsigned short)extPort);
		}
		else
			clog(NetWarn) << "Couldn't punch through NAT (or no NAT in place).";
	}

	return upnpEP;
}

bi::tcp::endpoint Network::resolveHost(string const& _addr)
{
	static boost::asio::io_service s_resolverIoService;

	vector<string> split;
	boost::split(split, _addr, boost::is_any_of(":"));
	unsigned port = dev::p2p::c_defaultIPPort;

	try
	{
		if (split.size() > 1)
			port = static_cast<unsigned>(stoi(split.at(1)));
	}
	catch(...) {}

	boost::system::error_code ec;
	bi::address address = bi::address::from_string(split[0], ec);
	bi::tcp::endpoint ep(bi::address(), port);
	if (!ec)
		ep.address(address);
	else
	{
		boost::system::error_code ec;
		// resolve returns an iterator (host can resolve to multiple addresses)
		bi::tcp::resolver r(s_resolverIoService);
		auto it = r.resolve({bi::tcp::v4(), split[0], toString(port)}, ec);
		if (ec)
		{
			clog(NetWarn) << "Error resolving host address..." << LogTag::Url << _addr << ":" << LogTag::Error << ec.message();
			return bi::tcp::endpoint();
		}
		else
			ep = *it;
	}
	return ep;
}

