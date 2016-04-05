/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Aleth.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <unordered_set>
#include <libethcore/KeyManager.h>
#include <libwebthree/WebThree.h>
#include "NatspecHandler.h"
#include "AlethFace.h"

namespace dev
{

namespace aleth
{

enum
{
	DefaultPasswordFlags = 0,
	NeedConfirm = 1,
	IsRetry = 2
};

static auto DoNotVerify = [](std::string const&){ return true; };

class Aleth: public AlethFace
{
	Q_OBJECT

public:
	enum OnInit
	{
		Nothing,
		OpenOnly,
		Bootstrap
	};

	explicit Aleth(QObject* _parent = nullptr);
	virtual ~Aleth();

	/// Initialise everything. Must be called first. Begins start()ed.
	void init(OnInit _open = OpenOnly, std::string const& _clientVersion = "unknown", std::string const& _nodeName = "anon");

	WebThreeDirect* web3() const override { return m_webThree.get(); }
	NatSpecFace& natSpec() override { return m_natSpecDB; }
	eth::KeyManager& keyManager() override { return m_keyManager; }
	Secret retrieveSecret(Address const& _address) const override;

	eth::ChainParams const& baseParams() const override { return m_baseParams; }

	/// Start the webthree subsystem.
	bool open(OnInit _open = Bootstrap);
	/// Stop the webthree subsystem.
	void close();

	// Deep Ethereum API
	virtual void reopenChain(eth::ChainParams const& _p) override;
	virtual void reopenChain() override;
	virtual bool isTampered() const override { return m_isTampered; }

	// Watch API
	unsigned installWatch(eth::LogFilter const& _tf, WatchHandler const& _f) override;
	unsigned installWatch(h256 const& _tf, WatchHandler const& _f) override;
	void uninstallWatch(unsigned _w) override;

	// Account naming API
	void install(AccountNamer* _adopt) override;
	void uninstall(AccountNamer* _kill) override;
	void noteKnownAddressesChanged(AccountNamer*) override;
	void noteAddressNamesChanged(AccountNamer*) override;
	Address toAddress(std::string const&) const override;
	std::string toName(Address const&) const override;
	Addresses allKnownAddresses() const override;

protected:
	virtual std::pair<std::string, bool> getPassword(std::string const& _prompt, std::string const& _title = std::string(), std::string const& _hint = std::string(), std::function<bool(std::string const&)> const& _verify = DoNotVerify, int _flags = DefaultPasswordFlags, std::string const& _failMessage = std::string()) const;

	virtual void openKeyManager();
	virtual void createKeyManager();

private slots:
	void checkHandlers();

private:
	std::unique_ptr<WebThreeDirect> m_webThree;
	eth::KeyManager m_keyManager;
	NatspecHandler m_natSpecDB;

	std::map<unsigned, WatchHandler> m_handlers;
	std::unordered_set<AccountNamer*> m_namers;

	bool m_destructing = false;
	eth::ChainParams m_baseParams;
	bool m_isTampered = false;
	std::string m_dbPath;
	std::string m_clientVersion;
	std::string m_nodeName;
};

}

}
