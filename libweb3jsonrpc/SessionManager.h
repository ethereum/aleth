#pragma once
#include <unordered_set>
#include <unordered_map>

#define RPC_ADMIN if (!m_sm.hasPrivilegeLevel(_session, Privilege::Admin)) throw jsonrpc::JsonRpcException("Invalid privileges");

namespace dev
{
namespace rpc
{

enum class Privilege
{
	Admin
};

}
}

namespace std
{
	template<> struct hash<dev::rpc::Privilege>
	{
		size_t operator()(dev::rpc::Privilege _value) const { return (size_t)_value; }
	};
}

namespace dev
{
namespace rpc
{

struct SessionPermissions
{
	std::unordered_set<Privilege> privileges;
};

class SessionManager
{
public:
	std::string newSession(SessionPermissions const& _p);
	void addSession(std::string const& _session, SessionPermissions const& _p);
	bool hasPrivilegeLevel(std::string const& _session, Privilege _l) const;

private:
	std::unordered_map<std::string, SessionPermissions> m_sessions;
};

}
}
