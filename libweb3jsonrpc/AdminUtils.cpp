#include <jsonrpccpp/common/exception.h>
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include "SessionManager.h"
#include "AdminUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::rpc;

AdminUtils::AdminUtils(SessionManager& _sm, SystemManager* _systemManager):
	m_sm(_sm),
	m_systemManager(_systemManager)
{}

bool AdminUtils::admin_setVerbosity(int _v, std::string const& _session)
{
	RPC_ADMIN;
	return admin_verbosity(_v);
}

bool AdminUtils::admin_verbosity(int _v)
{
	g_logVerbosity = _v;
	return true;
}

bool AdminUtils::admin_exit(std::string const& _session)
{
	RPC_ADMIN;
	if (m_systemManager)
	{
		m_systemManager->exit();
		return true;
	}
	return false;
}
