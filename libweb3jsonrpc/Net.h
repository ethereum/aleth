#pragma once
#include "NetFace.h"

namespace dev
{

class WebThreeNetworkFace;

namespace rpc
{

class Net: public NetFace
{
public:
	Net(WebThreeNetworkFace& _network);
	virtual std::string net_version() override;
	virtual std::string net_peerCount() override;
	virtual bool net_listening() override;

private:
	WebThreeNetworkFace& m_network;
};

}
}