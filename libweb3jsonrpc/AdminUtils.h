#pragma once
#include "AdminUtilsFace.h"

namespace dev
{
namespace rpc
{

class SessionManager;

class AdminUtils: public dev::rpc::AdminUtilsFace
{
public:
	AdminUtils(SessionManager& _sm);
	virtual bool admin_setVerbosity(int _v, std::string const& _session) override;
	virtual bool admin_exit(std::string const& _session) override;

private:
	SessionManager& m_sm;
};

}
}