#pragma once
#include "AdminUtilsFace.h"

namespace dev
{
namespace rpc
{

class SessionManager;

class SystemManager
{
public:
	virtual void exit() = 0;
};

class AdminUtils: public dev::rpc::AdminUtilsFace
{
public:
	AdminUtils(SessionManager& _sm, SystemManager* _systemManager = nullptr);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"admin", "1.0"}};
	}
	virtual bool admin_setVerbosity(int _v, std::string const& _session) override;
	virtual bool admin_verbosity(int _v) override;
	virtual bool admin_exit(std::string const& _session) override;

private:
	SessionManager& m_sm;
	SystemManager* m_systemManager = nullptr;
};

}
}
