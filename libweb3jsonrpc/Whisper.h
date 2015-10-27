#pragma once
#include <libdevcrypto/Common.h>
#include "WhisperFace.h"

namespace dev
{

class WebThreeDirect;

namespace shh
{
class Interface;
}

namespace rpc
{
	
class Whisper: public WhisperFace
{
public:
	// TODO: init with whisper interface instead of webthreedirect
	Whisper(WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts);
	
	virtual void setIdentities(std::vector<dev::KeyPair> const& _ids);

	virtual bool shh_post(Json::Value const& _json) override;
	virtual std::string shh_newIdentity() override;
	virtual bool shh_hasIdentity(std::string const& _identity) override;
	virtual std::string shh_newGroup(std::string const& _id, std::string const& _who) override;
	virtual std::string shh_addToGroup(std::string const& _group, std::string const& _who) override;
	virtual std::string shh_newFilter(Json::Value const& _json) override;
	virtual bool shh_uninstallFilter(std::string const& _filterId) override;
	virtual Json::Value shh_getFilterChanges(std::string const& _filterId) override;
	virtual Json::Value shh_getMessages(std::string const& _filterId) override;
	
private:
	shh::Interface* shh() const;

	WebThreeDirect& m_web3;
	std::map<dev::Public, dev::Secret> m_ids;
	std::map<unsigned, dev::Public> m_watches;
};

}
}