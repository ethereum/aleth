#include <libwebthree/WebThree.h>
#include <libdevcore/CommonJS.h>
#include <libethcore/Common.h>
#include "Net.h"

using namespace dev;
using namespace dev::rpc;

Net::Net(WebThreeNetworkFace& _network): m_network(_network) {}

std::string Net::net_version()
{
	return toJS((unsigned)eth::c_network);
}

std::string Net::net_peerCount()
{
	return toJS(m_network.peerCount());
}

bool Net::net_listening()
{
	return m_network.isNetworkStarted();
}
