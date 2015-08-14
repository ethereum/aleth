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
/** @file MainWin.h
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
#include <libdevcore/RLP.h>
#include <libethcore/Common.h>
#include <libethcore/KeyManager.h>
#include <libethereum/State.h>
#include <libethereum/Executive.h>
#include <libwebthree/WebThree.h>
#include <libsolidity/CompilerStack.h>
#include "Context.h"
#include "Transact.h"
#include "NatspecHandler.h"
#include "Connect.h"
#include "MainFace.h"

class QListWidgetItem;
class QActionGroup;

namespace Ui {
class Main;
}

namespace jsonrpc {
class HttpServer;
}

class QWebEnginePage;

namespace dev
{

namespace eth
{
class Client;
class State;
}

namespace az
{

class OurWebThreeStubServer;
class DappLoader;
class DappHost;
struct Dapp;

QString contentsOfQResource(std::string const& res);

class Main: public MainFace
{
	Q_OBJECT

public:
	explicit Main(QWidget *parent = 0);
	~Main();

	WebThreeDirect* web3() const override { return m_webThree.get(); }
	OurWebThreeStubServer* web3Server() const override { return m_server.get(); }
	eth::Client* ethereum() const override { return m_webThree->ethereum(); }
	std::shared_ptr<shh::WhisperHost> whisper() const override { return m_webThree->whisper(); }

	bool confirm() const;
	NatSpecFace* natSpec() { return &m_natSpecDB; }

	std::string pretty(dev::Address const& _a) const override;
	std::string prettyU256(u256 const& _n) const override;
	std::string render(dev::Address const& _a) const override;
	std::pair<Address, bytes> fromString(std::string const& _a) const override;
	std::string renderDiff(eth::StateDiff const& _d) const override;

	u256 gasPrice() const override;

	eth::KeyManager& keyManager() override { return m_keyManager; }
	void noteKeysChanged() override { refreshBalances(); }
	bool doConfirm();

	Secret retrieveSecret(Address const& _address) const override;

	// Account naming API.
	void install(AccountNamer* _adopt) override;
	void uninstall(AccountNamer* _kill) override;
	void noteKnownAddressesChanged(AccountNamer*) override;
	void noteAddressNamesChanged(AccountNamer*) override;
	Address toAddress(std::string const&) const override;
	std::string toName(Address const&) const override;
	Addresses allKnownAddresses() const override;

	void noteSettingsChanged() override { writeSettings(); }

public slots:
	void load(QString _file);
	void note(QString _entry);
	void debug(QString _entry);
	void warn(QString _entry);
	QString contents(QString _file);

	int authenticate(QString _title, QString _text);

	void onKeysChanged();

private slots:
	void eval(QString const& _js);
	void addConsoleMessage(QString const& _js, QString const& _s);

	// Application
	void on_about_triggered();
	void on_quit_triggered() { close(); }

	// Network
	void on_go_triggered();
	void on_net_triggered();
	void on_connect_triggered();
	void on_idealPeers_valueChanged(int);

	// Mining
	void on_mine_triggered();
	void on_prepNextDAG_triggered();

	// View
	void on_refresh_triggered();
	void on_showAll_triggered() { refreshBlockChain(); }
	void on_preview_triggered();

	// Account management
	void on_killAccount_triggered();
	void on_importKey_triggered();
	void on_reencryptKey_triggered();
	void on_reencryptAll_triggered();
	void on_importKeyFile_triggered();
	void on_claimPresale_triggered();
	void on_exportKey_triggered();

	// Tools
	void on_newTransaction_triggered();
	void on_loadJS_triggered();
	void on_exportState_triggered();

	// Stuff concerning the blocks/transactions/accounts panels
	void on_ourAccounts_itemClicked(QListWidgetItem* _i);
	void on_ourAccounts_doubleClicked();
	void on_transactionQueue_currentItemChanged();
	void on_blockChainFilter_textChanged();
	void on_blocks_currentItemChanged();

	// Misc
	void on_urlEdit_returnPressed();
	void on_jsInput_returnPressed();
	void on_nameReg_textChanged();

	// Special (debug) stuff
	void on_paranoia_triggered();
	void on_killBlockchain_triggered();
	void on_clearPending_triggered();
	void on_inject_triggered();
	void on_injectBlock_triggered();
	void on_forceMining_triggered();
	void on_usePrivate_triggered();
	void on_turboMining_triggered();
	void on_retryUnknown_triggered();
	void on_vmInterpreter_triggered();
	void on_vmJIT_triggered();
	void on_vmSmart_triggered();
	void on_rewindChain_triggered();

	// Debugger
	void on_debugCurrent_triggered();
	void on_debugDumpState_triggered() { debugDumpState(1); }
	void on_debugDumpStatePre_triggered() { debugDumpState(0); }
	void on_dumpBlockState_triggered();

	// Config
	void on_gasPrices_triggered();
	void on_sentinel_triggered();

	void refreshBlockChain();

	// Dapps
	void dappLoaded(Dapp& _dapp); //qt does not support rvalue refs for signals
	void pageLoaded(QByteArray const& _content, QString const& _mimeType, QUrl const& _uri);

signals:
	void poll();

private:
	template <class P> void loadPlugin() { Plugin* p = new P(this); initPlugin(p); }
	void initPlugin(Plugin* _p);
	void finalisePlugin(Plugin* _p);
	void unloadPlugin(std::string const& _name);

	void debugDumpState(int _add);

	p2p::NetworkPreferences netPrefs() const;

	QString lookup(QString const& _n) const;
	Address getNameReg() const;
	Address getCurrencies() const;

	void updateFee();
	void readSettings(bool _skipGeometry = false);
	void writeSettings();

	void setPrivateChain(QString const& _private, bool _forceConfigure = false);

	unsigned installWatch(eth::LogFilter const& _tf, WatchHandler const& _f) override;
	unsigned installWatch(h256 const& _tf, WatchHandler const& _f) override;
	void uninstallWatch(unsigned _w) override;

	void keysChanged();

	void onNewPending();
	void onNewBlock();
	void onNameRegChange();
	void onCurrenciesChange();
	void onBalancesChange();

	void installWatches();
	void installCurrenciesWatch();
	void installNameRegWatch();
	void installBalancesWatch();

	virtual void timerEvent(QTimerEvent*) override;

	void refreshNetwork();
	void refreshMining();
	void refreshCache();

	void refreshAll();
	void refreshPending();
	void refreshBlockCount();
	void refreshBalances();

	void setBeneficiary(Address const& _b);
	std::string getPassword(std::string const& _title, std::string const& _for, std::string* _hint = nullptr, bool* _ok = nullptr);

	std::unique_ptr<Ui::Main> ui;

	std::unique_ptr<WebThreeDirect> m_webThree;

	std::map<unsigned, WatchHandler> m_handlers;
	unsigned m_nameRegFilter = (unsigned)-1;
	unsigned m_currenciesFilter = (unsigned)-1;
	unsigned m_balancesFilter = (unsigned)-1;

	QByteArray m_networkConfig;
	QStringList m_servers;
	eth::KeyManager m_keyManager;
	QString m_privateChain;
	dev::Address m_nameReg;
	dev::Address m_beneficiary;

	QActionGroup* m_vmSelectionGroup = nullptr;

	QList<QPair<QString, QString>> m_consoleHistory;

	std::unique_ptr<jsonrpc::HttpServer> m_httpConnector;
	std::unique_ptr<OurWebThreeStubServer> m_server;

	static std::string fromRaw(h256 const& _n, unsigned* _inc = nullptr);
	NatspecHandler m_natSpecDB;

	Transact* m_transact;
	std::unique_ptr<DappHost> m_dappHost;
	DappLoader* m_dappLoader;
	QWebEnginePage* m_webPage;

	Connect m_connect;

	std::unordered_set<AccountNamer*> m_namers;

	bool m_destructing = false;
};

}
}
