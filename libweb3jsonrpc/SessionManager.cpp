#include <libdevcore/Base64.h>
#include "SessionManager.h"

using namespace std;
using namespace dev;
using namespace dev::rpc;

std::string SessionManager::newSession(SessionPermissions const& _p)
{
	std::string s = toBase64(h64::random().ref());
	m_sessions[s] = _p;
	return s;
}

void SessionManager::addSession(std::string const& _session, SessionPermissions const& _p)
{
	m_sessions[_session] = _p;
}

bool SessionManager::hasPrivilegeLevel(std::string const& _session, Privilege _l) const
{
	auto it = m_sessions.find(_session);
	return it != m_sessions.end() && it->second.privileges.count(_l);
}
