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
/** @file AlethZero.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#ifdef Q_MOC_RUN
#define BOOST_MPL_IF_HPP_INCLUDED
#endif

#include <map>
#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QAbstractListModel>
#include <QtCore/QMutex>
#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <libdevcore/RLP.h>
#include <libethcore/Common.h>
#include <libethcore/KeyManager.h>
#include <libethereum/State.h>
#include <libethereum/Executive.h>
#include <libwebthree/WebThree.h>
#if ETH_SOLIDITY
#include <libsolidity/interface/CompilerStack.h>
#endif
#include <libaleth/NatspecHandler.h>
#include <libaleth/Common.h>
#include <libaleth/Aleth.h>
#include <libaleth/RPCHost.h>
#include "ZeroFace.h"
#include "Plugin.h"

class QListWidgetItem;
class QActionGroup;

namespace Ui { class Main; }

namespace dev
{

namespace eth
{
class Client;
class State;
}

namespace aleth
{
namespace zero
{

class SettingsDialog;
struct NetworkSettings;

class AlethZero: public ZeroFace
{
	Q_OBJECT

public:
	AlethZero();
	~AlethZero();

	SafeHttpServer* web3ServerConnector() const override { return m_rpcHost.httpConnector(); }
	AlethWhisper* whisperFace() const override { return m_rpcHost.whisperFace(); }
	rpc::SessionManager* sessionManager() const override { return m_rpcHost.sessionManager(); }

	AlethFace const* aleth() const override { return &m_aleth; }
	AlethFace* aleth() override { return &m_aleth; }

public slots:
	void note(QString _entry);
	void debug(QString _entry);
	void warn(QString _entry);
	QString contents(QString _file);

	void onKeysChanged();

private slots:
	// Application
	void on_about_triggered();
	void on_quit_triggered() { close(); }

	// Network
	void on_go_triggered();
	void on_net_triggered();
	void on_connect_triggered();

	// View
	void on_refresh_triggered();
	void on_preview_triggered();

	// Account management
	void on_killAccount_triggered();
	void on_reencryptKey_triggered();
	void on_reencryptAll_triggered();
	void on_exportKey_triggered();
	void on_ourAccounts_itemClicked(QListWidgetItem* _i);
	void on_ourAccounts_doubleClicked();

	// Special (debug) stuff
	void on_confirm_triggered();
	void on_settings_triggered();
	void refreshAll();

	void onBeneficiaryChanged();

private:
	void allStop() override;
	void carryOn() override;

	template <class P> void loadPlugin() { Plugin* p = new P(this); initPlugin(p); }
	void initPlugin(Plugin* _p);
	void finalisePlugin(Plugin* _p);
	void unloadPlugin(std::string const& _name);

	void addSettingsPage(int _index, QString const& _categoryName, std::function<SettingsPage*()> const& _pageFactory) override;

	void createSettingsPages();
	void setNetPrefs(NetworkSettings const& _settings);
	NetworkSettings netPrefs() const;

	void readSettings(bool _skipGeometry = false, bool _onlyGeometry = false);
	void writeSettings() override;

	// Probably unnecessary once everything is pluginified.
	void installWatches();

	// TODO remove once we have a separate network activity panel/page
	void refreshNetwork();

	// TODO should become a plugin
	void refreshMining();

	// TODO should become a plugin
	void refreshBlockCount();

	// TODO should become a plugin
	void refreshBalances();

	// TODO remove once all account management is removed.
	std::string getPassword(std::string const& _title, std::string const& _for, std::string* _hint = nullptr, bool* _ok = nullptr);

	std::unique_ptr<Ui::Main> m_ui;
	SettingsDialog* m_settingsDialog = nullptr;

	Aleth m_aleth;
	RPCHost m_rpcHost;
};

}
}
}
