#include <jsonrpccpp/common/exception.h>
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include "SessionManager.h"
#include "AdminUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::rpc;

AdminUtils::AdminUtils(SessionManager& _sm): m_sm(_sm){}

bool AdminUtils::admin_setVerbosity(int _v, std::string const& _session)
{
	RPC_ADMIN;
	g_logVerbosity = _v;
	return true;
}

bool AdminUtils::admin_exit(std::string const& _session)
{
	RPC_ADMIN;
	Client::exitHandler(SIGTERM);
	return true;
}