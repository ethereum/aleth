// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// UPnP port forwarding support.
#include "UPnP.h"

#include <string.h>
#if ETH_MINIUPNPC
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#endif
#include <libdevcore/Exceptions.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
using namespace std;
using namespace dev;
using namespace dev::p2p;

UPnP::UPnP()
{
#if ETH_MINIUPNPC
	m_urls = make_shared<UPNPUrls>();
	m_data = make_shared<IGDdatas>();

	m_ok = false;

	struct UPNPDev* devlist;
	struct UPNPDev* dev;
	char* descXML;
	int descXMLsize = 0;
	int upnperror = 0;
	memset(m_urls.get(), 0, sizeof(struct UPNPUrls));
	memset(m_data.get(), 0, sizeof(struct IGDdatas));
#if MINIUPNPC_API_VERSION >= 14
	devlist = upnpDiscover(2000, NULL/*multicast interface*/, NULL/*minissdpd socket path*/, 0/*sameport*/, 0/*ipv6*/, 2/*ttl*/, &upnperror);
#else
	devlist = upnpDiscover(2000, NULL/*multicast interface*/, NULL/*minissdpd socket path*/, 0/*sameport*/, 0/*ipv6*/, &upnperror);
#endif
	if (devlist)
	{
		dev = devlist;
		while (dev)
		{
			if (strstr (dev->st, "InternetGatewayDevice"))
				break;
			dev = dev->pNext;
		}
		if (!dev)
			dev = devlist; /* defaulting to first device */

		cnote << "UPnP device:" << dev->descURL << "[st:" << dev->st << "]";
#if MINIUPNPC_API_VERSION >= 16
		int responsecode = 200;
		descXML = (char*)miniwget(dev->descURL, &descXMLsize, 0, &responsecode);
#elif MINIUPNPC_API_VERSION >= 9
		descXML = (char*)miniwget(dev->descURL, &descXMLsize, 0);
#else
		descXML = (char*)miniwget(dev->descURL, &descXMLsize);
#endif
		if (descXML)
		{
			parserootdesc(descXML, descXMLsize, m_data.get());
			free(descXML);
#if MINIUPNPC_API_VERSION >= 9
			GetUPNPUrls (m_urls.get(), m_data.get(), dev->descURL, 0);
#else
			GetUPNPUrls (m_urls.get(), m_data.get(), dev->descURL);
#endif
			m_ok = true;
		}
		freeUPNPDevlist(devlist);
	}
	else
#endif
	{
		cnote << "UPnP device not found.";
		BOOST_THROW_EXCEPTION(NoUPnPDevice());
	}
}

UPnP::~UPnP()
{
	auto r = m_reg;
	for (auto i: r)
		removeRedirect(i);
}

string UPnP::externalIP()
{
#if ETH_MINIUPNPC
	char addr[16];
	if (!UPNP_GetExternalIPAddress(m_urls->controlURL, m_data->first.servicetype, addr))
		return addr;
#endif
	return "0.0.0.0";
}

int UPnP::addRedirect(char const* _addr, int _port)
{
	(void)_addr;
	(void)_port;
#if ETH_MINIUPNPC
	if (m_urls->controlURL[0] == '\0')
	{
		cwarn << "UPnP::addRedirect() called without proper initialisation?";
		return -1;
	}

	// Try direct mapping first (port external, port internal).
	char port_str[16];
	char ext_port_str[16];
	sprintf(port_str, "%d", _port);
	if (!UPNP_AddPortMapping(m_urls->controlURL, m_data->first.servicetype, port_str, port_str, _addr, "ethereum", "TCP", NULL, NULL))
		return _port;

	// Failed - now try (random external, port internal) and cycle up to 10 times.
	srand(static_cast<unsigned int>(time(nullptr)));
	for (unsigned i = 0; i < 10; ++i)
	{
		_port = rand() % (32768 - 1024) + 1024;
		sprintf(ext_port_str, "%d", _port);
		if (!UPNP_AddPortMapping(m_urls->controlURL, m_data->first.servicetype, ext_port_str, port_str, _addr, "ethereum", "TCP", NULL, NULL))
			return _port;
	}

	// Failed. Try asking the router to give us a free external port.
	if (UPNP_AddPortMapping(m_urls->controlURL, m_data->first.servicetype, port_str, NULL, _addr, "ethereum", "TCP", NULL, NULL))
		// Failed. Exit.
		return 0;

	// We got mapped, but we don't know which ports we got mapped to. Now to find...
	unsigned num = 0;
	UPNP_GetPortMappingNumberOfEntries(m_urls->controlURL, m_data->first.servicetype, &num);
	for (unsigned i = 0; i < num; ++i)
	{
		char extPort[16];
		char intClient[16];
		char intPort[6];
		char protocol[4];
		char desc[80];
		char enabled[4];
		char rHost[64];
		char duration[16];
		UPNP_GetGenericPortMappingEntry(m_urls->controlURL, m_data->first.servicetype, toString(i).c_str(), extPort, intClient, intPort, protocol, desc, enabled, rHost, duration);
		if (string("ethereum") == desc)
		{
			m_reg.insert(atoi(extPort));
			return atoi(extPort);
		}
	}
	cerr << "ERROR: Mapped port not found." << endl;
#endif
	return 0;
}

void UPnP::removeRedirect(int _port)
{
	(void)_port;
#if ETH_MINIUPNPC
	char port_str[16];
	printf("TB : upnp_rem_redir (%d)\n", _port);
	if (m_urls->controlURL[0] == '\0')
	{
		printf("TB : the init was not done !\n");
		return;
	}
	sprintf(port_str, "%d", _port);
	UPNP_DeletePortMapping(m_urls->controlURL, m_data->first.servicetype, port_str, "TCP", NULL);
	m_reg.erase(_port);
#endif
}
