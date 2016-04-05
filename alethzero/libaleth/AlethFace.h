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
/** @file AlethFace.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <functional>
#include <QObject>
#include <libevm/ExtVMFace.h>
#include <libethereum/ChainParams.h>
#include "Context.h"
#include "Common.h"
#include "AccountNamer.h"

namespace dev
{

class WebThreeDirect;
namespace eth { class Client; class LogFilter; class KeyManager; }
namespace shh { class WhisperHost; }

namespace aleth
{

using WatchHandler = std::function<void(dev::eth::LocalisedLogEntries const&)>;

class AlethFace: public QObject, public Context
{
	Q_OBJECT

public:
	AlethFace(QObject* _parent = nullptr): QObject(_parent) {}

	// TODO: tidy - all should be references that throw if module unavailable.
	// TODO: provide a set of available web3 modules.
	bool isOpen() const { return !!web3(); }
	explicit operator bool() const { return isOpen(); }
	virtual dev::WebThreeDirect* web3() const = 0;
	dev::eth::Client* ethereum() const;
	std::shared_ptr<dev::shh::WhisperHost> whisper() const;

	Address author() const;
	void setAuthor(Address const& _a);

	virtual NatSpecFace& natSpec() = 0;

	// Deep Ethereum API
	virtual eth::ChainParams const& baseParams() const = 0;
	virtual void reopenChain(eth::ChainParams const& _p) = 0;
	virtual void reopenChain() = 0;
	virtual bool isTampered() const = 0;

	// Watch API
	virtual unsigned installWatch(dev::eth::LogFilter const& _tf, WatchHandler const& _f) = 0;
	virtual unsigned installWatch(dev::h256 const& _tf, WatchHandler const& _f) = 0;
	virtual void uninstallWatch(unsigned _id) = 0;

	// Account naming API
	virtual void install(AccountNamer* _adopt) = 0;
	virtual void uninstall(AccountNamer* _kill) = 0;
	virtual void noteKnownAddressesChanged(AccountNamer*) = 0;
	virtual void noteAddressNamesChanged(AccountNamer*) = 0;

	// Additional address lookup API
	std::string toReadable(Address const& _a) const;
	std::pair<Address, bytes> readAddress(std::string const& _n) const;
	std::string toHTML(eth::StateDiff const& _d) const;
	using Context::toHTML;

	// DNS lookup API
	std::string dnsLookup(std::string const& _a) const;

	// Key managing API
	virtual dev::Secret retrieveSecret(dev::Address const& _a) const = 0;
	virtual dev::eth::KeyManager& keyManager() = 0;
	virtual void noteKeysChanged() { emit keysChanged(); }	// TODO: remove and use keyManager directly, once it supports C++ function signals.

	virtual void noteSettingsChanged() { emit settingsChanged(); }

signals:
	void knownAddressesChanged();
	void addressNamesChanged();

	void beneficiaryChanged();

	void keysChanged();
	void settingsChanged();
};

}

}
