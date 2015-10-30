#pragma once
#include "AdminEthFace.h"

namespace dev
{

namespace eth
{
class Client;
class TrivialGasPricer;
class KeyManager;
}

namespace rpc
{

class SessionManager;

class AdminEth: public AdminEthFace
{
public:
	AdminEth(eth::Client& _eth, eth::TrivialGasPricer& _gp, eth::KeyManager& _keyManager, SessionManager& _sm);

	virtual bool admin_eth_setMining(bool _on, std::string const& _session) override;
	virtual Json::Value admin_eth_blockQueueStatus(std::string const& _session) override;
	virtual bool admin_eth_setAskPrice(std::string const& _wei, std::string const& _session) override;
	virtual bool admin_eth_setBidPrice(std::string const& _wei, std::string const& _session) override;
	virtual Json::Value admin_eth_findBlock(std::string const& _blockHash, std::string const& _session) override;
	virtual std::string admin_eth_blockQueueFirstUnknown(std::string const& _session) override;
	virtual bool admin_eth_blockQueueRetryUnknown(std::string const& _session) override;
	virtual Json::Value admin_eth_allAccounts(std::string const& _session) override;
	virtual Json::Value admin_eth_newAccount(const Json::Value& _info, std::string const& _session) override;
	virtual bool admin_eth_setMiningBenefactor(std::string const& _uuidOrAddress, std::string const& _session) override;
	virtual Json::Value admin_eth_inspect(std::string const& _address, std::string const& _session) override;
	virtual Json::Value admin_eth_reprocess(std::string const& _blockNumberOrHash, std::string const& _session) override;
	virtual Json::Value admin_eth_vmTrace(std::string const& _blockNumberOrHash, int _txIndex, std::string const& _session) override;
	virtual Json::Value admin_eth_getReceiptByHashAndIndex(std::string const& _blockNumberOrHash, int _txIndex, std::string const& _session) override;

	virtual void setMiningBenefactorChanger(std::function<void(Address const&)> const& _f) { m_setMiningBenefactor = _f; }
private:
	eth::Client& m_eth;
	eth::TrivialGasPricer& m_gp;
	eth::KeyManager& m_keyManager;
	SessionManager& m_sm;
	std::function<void(Address const&)> m_setMiningBenefactor;

	h256 blockHash(std::string const& _blockNumberOrHash) const;
};

}
}
