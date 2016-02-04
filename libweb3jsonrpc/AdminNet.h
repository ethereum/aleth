#pragma once
#include "AdminNetFace.h"

namespace dev
{

class NetworkFace;

namespace rpc
{

class SessionManager;

class AdminNet: public dev::rpc::AdminNetFace
{
public:
	AdminNet(NetworkFace& _network, SessionManager& _sm);
	virtual std::pair<std::string, std::string> implementedModule() const override
	{
		return std::make_pair(std::string("admin"), std::string("1.0"));
	}
	virtual bool admin_net_start(std::string const& _session) override;
	virtual bool admin_net_stop(std::string const& _session) override;
	virtual bool admin_net_connect(std::string const& _node, std::string const& _session) override;
	virtual Json::Value admin_net_peers(std::string const& _session) override;
	virtual Json::Value admin_net_nodeInfo(std::string const& _session) override;
	virtual Json::Value admin_nodeInfo() override;

private:
	NetworkFace& m_network;
	SessionManager& m_sm;
};
	
}
}
