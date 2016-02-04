#pragma once
#include <libwebthree/SystemManager.h>
#include "AdminUtilsFace.h"

namespace dev
{
namespace rpc
{

class SessionManager;

class AdminUtils: public dev::rpc::AdminUtilsFace
{
public:
	AdminUtils(SessionManager& _sm, SystemManager* _systemManager = nullptr);
	virtual std::pair<std::string, std::string> implementedModule() const override
	{
		return std::make_pair(std::string("admin"), std::string("1.0"));
	}
	virtual bool admin_setVerbosity(int _v, std::string const& _session) override;
	virtual bool admin_exit(std::string const& _session) override;

private:
	SessionManager& m_sm;
	SystemManager* m_systemManager = nullptr;
};

}
}
