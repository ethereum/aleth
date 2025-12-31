// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <array>
#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include "Common.h"
namespace ba = boost::asio;
namespace bi = ba::ip;

namespace dev
{
namespace p2p
{

constexpr const char* c_localhostIp = "127.0.0.1";
constexpr unsigned short c_defaultListenPort = 30303;

struct NetworkConfig
{
    // Default Network Preferences
    explicit NetworkConfig(unsigned short _listenPort = c_defaultListenPort)
      : listenPort(_listenPort)
    {}

    // Network Preferences with specific Listen IP
    NetworkConfig(std::string const& _listenAddress,
        unsigned short _listenPort = c_defaultListenPort, bool _upnp = true,
        bool _allowLocalDiscovery = false)
      : publicIPAddress(),
        listenIPAddress(_listenAddress),
        listenPort(_listenPort),
        traverseNAT(_upnp),
        allowLocalDiscovery(_allowLocalDiscovery)
    {}

    // Network Preferences with intended Public IP
    NetworkConfig(std::string const& _publicIP, std::string const& _listenAddress = std::string(),
        unsigned short _listenPort = c_defaultListenPort, bool _upnp = true,
        bool _allowLocalDiscovery = false)
      : publicIPAddress(_publicIP),
        listenIPAddress(_listenAddress),
        listenPort(_listenPort),
        traverseNAT(_upnp),
        allowLocalDiscovery(_allowLocalDiscovery)
    {
        if (!publicIPAddress.empty() && !isPublicAddress(publicIPAddress))
            BOOST_THROW_EXCEPTION(InvalidPublicIPAddress());
    }

    /// Addressing

	std::string publicIPAddress;
	std::string listenIPAddress;
	unsigned short listenPort = c_defaultListenPort;


	/// Preferences

	bool traverseNAT = true;
	bool discovery = true;		// Discovery is activated with network.
	bool allowLocalDiscovery = false; // Include nodes with local IP addresses in the discovery process.
	bool pin = false;			// Only accept or connect to trusted peers.
};

/**
 * @brief Network Class
 * Static network operations and interface(s).
 */
class Network
{
public:
	/// @returns public and private interface addresses
	static std::set<bi::address> getInterfaceAddresses();

	/// Try to bind and listen on _listenPort, else attempt net-allocated port.
	static int tcp4Listen(bi::tcp::acceptor& _acceptor, NetworkConfig const& _config);

	/// Return public endpoint of upnp interface. If successful o_upnpifaddr will be a private interface address and endpoint will contain public address and port.
	static bi::tcp::endpoint traverseNAT(std::set<bi::address> const& _ifAddresses, unsigned short _listenPort, bi::address& o_upnpInterfaceAddr);

	/// Resolve "host:port" string as TCP endpoint. Returns unspecified endpoint on failure.
	static bi::tcp::endpoint resolveHost(std::string const& _host);
};

}
}
