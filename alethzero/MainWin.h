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
#include <libethereum/State.h>
#include <libethereum/Executive.h>
#include <libwebthree/WebThree.h>
#include <libsolidity/CompilerStack.h>
#include "Context.h"
#include "Transact.h"
#include "NatspecHandler.h"
#include "Connect.h"

namespace Ui {
class Main;
}

namespace dev { namespace eth {
class Client;
class State;
}}

namespace jsonrpc {
class HttpServer;
}

class QWebEnginePage;
class OurWebThreeStubServer;
class DappLoader;
class DappHost;
struct Dapp;

using WatchHandler = std::function<void(dev::eth::LocalisedLogEntries const&)>;

QString contentsOfQResource(std::string const& res);

class Main: public QMainWindow, public Context
{
	Q_OBJECT

public:
	explicit Main(QWidget *parent = 0);
	~Main();

	dev::WebThreeDirect* web3() const { return m_webThree.get(); }
	dev::eth::Client* ethereum() const { return m_webThree->ethereum(); }
	std::shared_ptr<dev::shh::WhisperHost> whisper() const { return m_webThree->whisper(); }

	bool confirm() const;
	NatSpecFace* natSpec() { return &m_natSpecDB; }

	QString pretty(dev::Address _a) const override;
	QString prettyU256(dev::u256 _n) const override;
	QString render(dev::Address _a) const override;
	dev::Address fromString(QString const& _a) const override;
	std::string renderDiff(dev::eth::StateDiff const& _d) const override;

	QList<dev::KeyPair> owned() const { return m_myIdentities + m_myKeys; }

	dev::u256 gasPrice() const { return 10 * dev::eth::szabo; }

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

	// View
	void on_refresh_triggered();
	void on_showAll_triggered() { refreshBlockChain(); }
	void on_showAllAccounts_triggered() { refreshAccounts(); }
	void on_preview_triggered();

	// Account management
	void on_newAccount_triggered();
	void on_killAccount_triggered();
	void on_importKey_triggered();
	void on_importKeyFile_triggered();
	void on_exportKey_triggered();

	// Tools
	void on_newTransaction_triggered();
	void on_loadJS_triggered();

	// Stuff concerning the blocks/transactions/accounts panels
	void ourAccountsRowsMoved();
	void on_ourAccounts_doubleClicked();
	void on_accounts_doubleClicked();
	void on_contracts_doubleClicked();
	void on_contracts_currentItemChanged();
	void on_transactionQueue_currentItemChanged();
	void on_blockChainFilter_textChanged();
	void on_blocks_currentItemChanged();

	// Logging
//	void on_log_doubleClicked();
	void on_verbosity_valueChanged();

	// Misc
	void on_urlEdit_returnPressed();
	void on_jsInput_returnPressed();
	void on_nameReg_textChanged();

	// Special (debug) stuff
	void on_paranoia_triggered();
	void on_killBlockchain_triggered();
	void on_clearPending_triggered();
	void on_inject_triggered();
	void on_forceMining_triggered();
	void on_usePrivate_triggered();
	void on_turboMining_triggered();
	void on_jitvm_triggered();

	// Debugger
	void on_debugCurrent_triggered();
	void on_debugDumpState_triggered() { debugDumpState(1); }
	void on_debugDumpStatePre_triggered() { debugDumpState(0); }

	// Whisper
	void on_newIdentity_triggered();
	void on_post_clicked();

	void refreshWhisper();
	void refreshBlockChain();
	void addNewId(QString _ids);

	// Dapps
	void dappLoaded(Dapp& _dapp); //qt does not support rvalue refs for signals

signals:
	void poll();

private:
	void debugDumpState(int _add);

	dev::p2p::NetworkPreferences netPrefs() const;

	QString lookup(QString const& _n) const;
	dev::Address getNameReg() const;
	dev::Address getCurrencies() const;

	void updateFee();
	void readSettings(bool _skipGeometry = false);
	void writeSettings();

	unsigned installWatch(dev::eth::LogFilter const& _tf, WatchHandler const& _f);
	unsigned installWatch(dev::h256 _tf, WatchHandler const& _f);
	void uninstallWatch(unsigned _w);

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

	virtual void timerEvent(QTimerEvent*);

	void refreshNetwork();
	void refreshMining();
	void refreshWhispers();
	void refreshCache();

	void refreshAll();
	void refreshPending();
	void refreshAccounts();
	void refreshBlockCount();
	void refreshBalances();

	std::unique_ptr<Ui::Main> ui;

	std::unique_ptr<dev::WebThreeDirect> m_webThree;

	std::map<unsigned, WatchHandler> m_handlers;
	unsigned m_nameRegFilter = (unsigned)-1;
	unsigned m_currenciesFilter = (unsigned)-1;
	unsigned m_balancesFilter = (unsigned)-1;

	QByteArray m_networkConfig;
	QStringList m_servers;
	QList<dev::KeyPair> m_myKeys;
	QList<dev::KeyPair> m_myIdentities;
	QString m_privateChain;
	dev::Address m_nameReg;

	QList<QPair<QString, QString>> m_consoleHistory;
	QMutex m_logLock;
	QString m_logHistory;
	bool m_logChanged = true;

	std::unique_ptr<jsonrpc::HttpServer> m_httpConnector;
	std::unique_ptr<OurWebThreeStubServer> m_server;

	static QString fromRaw(dev::h256 _n, unsigned* _inc = nullptr);
	NatspecHandler m_natSpecDB;

	Transact m_transact;
	std::unique_ptr<DappHost> m_dappHost;
	DappLoader* m_dappLoader;
	QWebEnginePage* m_webPage;
	
	Connect m_connect;
};
