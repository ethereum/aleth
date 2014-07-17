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
/** @file MainWin.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <fstream>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWebKitWidgets/QWebFrame>
#include <QtGui/QClipboard>
#include <QtCore/QtCore>
#include <boost/algorithm/string.hpp>
#include <libserpent/funcs.h>
#include <libserpent/util.h>
#include <libethcore/Dagger.h>
#include <liblll/Compiler.h>
#include <liblll/CodeFragment.h>
#include <libevm/VM.h>
#include <libethereum/BlockChain.h>
#include <libethereum/ExtVM.h>
#include <libethereum/Client.h>
#include <libethereum/PeerServer.h>
#include "MiningView.h"
#include "BuildInfo.h"
#include "MainWin.h"
#include "ui_Main.h"
using namespace std;

// types
using eth::bytes;
using eth::bytesConstRef;
using eth::h160;
using eth::h256;
using eth::u160;
using eth::u256;
using eth::Address;
using eth::BlockInfo;
using eth::Client;
using eth::Instruction;
using eth::KeyPair;
using eth::NodeMode;
using eth::BlockChain;
using eth::PeerInfo;
using eth::RLP;
using eth::Secret;
using eth::Transaction;
using eth::Executive;

// functions
using eth::toHex;
using eth::compileLLL;
using eth::disassemble;
using eth::formatBalance;
using eth::fromHex;
using eth::sha3;
using eth::left160;
using eth::right160;
using eth::simpleDebugOut;
using eth::toLog2;
using eth::toString;
using eth::units;
using eth::operator<<;

// vars
using eth::g_logPost;
using eth::g_logVerbosity;
using eth::c_instructionInfo;

// Horrible global for the mainwindow. Needed for the QEthereums to find the Main window which acts as multiplexer for now.
// Can get rid of this once we've sorted out ITC for signalling & multiplexed querying.
Main* g_main = nullptr;

static void initUnits(QComboBox* _b)
{
	for (auto n = (::uint)units().size(); n-- != 0; )
		_b->addItem(QString::fromStdString(units()[n].second), n);
}

Address c_config = Address("661005d2720d855f1d9976f88bb10c1a3398c77f");

Main::Main(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::Main)
{
	g_main = this;

	setWindowFlags(Qt::Window);
	ui->setupUi(this);
	g_logPost = [=](std::string const& s, char const* c) { simpleDebugOut(s, c); ui->log->addItem(QString::fromStdString(s)); };
	m_client.reset(new Client("AlethZero"));

	m_refresh = new QTimer(this);
	connect(m_refresh, SIGNAL(timeout()), SLOT(refresh()));
	m_refresh->start(100);
	m_refreshNetwork = new QTimer(this);
	connect(m_refreshNetwork, SIGNAL(timeout()), SLOT(refreshNetwork()));
	m_refreshNetwork->start(1000);
	m_refreshMining = new QTimer(this);
	connect(m_refreshMining, SIGNAL(timeout()), SLOT(refreshMining()));
	m_refreshMining->start(200);

	connect(ui->ourAccounts->model(), SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)), SLOT(ourAccountsRowsMoved()));

#if 0&&ETH_DEBUG
	m_servers.append("192.168.0.10:30301");
#else
    int pocnumber = QString(eth::EthVersion).section('.', 1, 1).toInt();
	if (pocnumber == 4)
		m_servers.push_back("54.72.31.55:30303");
	else if (pocnumber == 5)
        m_servers.push_back("54.72.69.180:30303");
	else
	{
		connect(&m_webCtrl, &QNetworkAccessManager::finished, [&](QNetworkReply* _r)
		{
			m_servers = QString::fromUtf8(_r->readAll()).split("\n", QString::SkipEmptyParts);
		});
		QNetworkRequest r(QUrl("http://www.ethereum.org/servers.poc" + QString::number(pocnumber) + ".txt"));
		r.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/33.0.1712.0 Safari/537.36");
		m_webCtrl.get(r);
		srand(time(0));
	}
#endif

    cerr << "State root: " << BlockChain::genesis().stateRoot << endl;
    cerr << "Block Hash: " << sha3(BlockChain::createGenesisBlock()) << endl;
    cerr << "Block RLP: " << RLP(BlockChain::createGenesisBlock()) << endl;
    cerr << "Block Hex: " << toHex(BlockChain::createGenesisBlock()) << endl;
	cerr << "Network protocol version: " << eth::c_protocolVersion << endl;

	ui->configDock->close();

	on_verbosity_valueChanged();
	initUnits(ui->gasPriceUnits);
	initUnits(ui->valueUnits);
	ui->valueUnits->setCurrentIndex(6);
	ui->gasPriceUnits->setCurrentIndex(4);
	ui->gasPrice->setValue(10);
	on_destination_currentTextChanged();

	statusBar()->addPermanentWidget(ui->balance);
	statusBar()->addPermanentWidget(ui->peerCount);
	statusBar()->addPermanentWidget(ui->mineStatus);
	statusBar()->addPermanentWidget(ui->blockCount);

	connect(ui->webView, &QWebView::titleChanged, [=]()
	{
		ui->tabWidget->setTabText(0, ui->webView->title());
	});

	connect(ui->webView, &QWebView::loadFinished, [=]()
	{
		this->changed();
	});

	QWebFrame* f = ui->webView->page()->currentFrame();
	connect(f, &QWebFrame::javaScriptWindowObjectCleared, [=](){
		auto qe = new QEthereum(this, m_client.get(), owned());
		qe->setup(f);
		f->addToJavaScriptWindowObject("env", this, QWebFrame::QtOwnership);
	});

	readSettings();
	refresh();

	{
		QSettings s("ethereum", "alethzero");
		if (s.value("splashMessage", true).toBool())
		{
			QMessageBox::information(this, "Here Be Dragons!", "This is proof-of-concept software. The project as a whole is not even at the alpha-testing stage. It is here to show you, if you have a technical bent, the sort of thing that might be possible down the line.\nPlease don't blame us if it does something unexpected or if you're underwhelmed with the user-experience. We have great plans for it in terms of UX down the line but right now we just want to get the groundwork sorted. We welcome contributions, be they in code, testing or documentation!\nAfter you close this message it won't appear again.");
			s.setValue("splashMessage", false);
		}
	}
	m_pcWarp.clear();
}

Main::~Main()
{
	g_logPost = simpleDebugOut;
	writeSettings();
}

void Main::on_clearPending_triggered()
{
	m_client->clearPending();
}

void Main::load(QString _s)
{
	QFile fin(_s);
	if (!fin.open(QFile::ReadOnly))
		return;
	QString line;
	while (!fin.atEnd())
	{
		QString l = QString::fromUtf8(fin.readLine());
		line.append(l);
		if (line.count('"') % 2)
		{
			line.chop(1);
		}
		else if (line.endsWith("\\\n"))
			line.chop(2);
		else
		{
			ui->webView->page()->currentFrame()->evaluateJavaScript(line);
			//eval(line);
			line.clear();
		}
	}
}
// env.load("/home/gav/eth/init.eth")

void Main::on_loadJS_triggered()
{
	QString f = QFileDialog::getOpenFileName(this, "Load Javascript", QString(), "Javascript (*.js);;All files (*)");
	if (f.size())
		load(f);
}

void Main::note(QString _s)
{
	cnote << _s.toStdString();
}

void Main::debug(QString _s)
{
	cdebug << _s.toStdString();
}

void Main::warn(QString _s)
{
	cwarn << _s.toStdString();
}

void Main::on_jsInput_returnPressed()
{
	eval(ui->jsInput->text());
	ui->jsInput->setText("");
}

void Main::eval(QString const& _js)
{
	if (_js.trimmed().isEmpty())
		return;
	QVariant ev = ui->webView->page()->currentFrame()->evaluateJavaScript(_js);
	QString s;
	if (ev.isNull())
		s = "<span style=\"color: #888\">null</span>";
	else if (ev.type() == QVariant::String)
		s = "<span style=\"color: #444\">\"</span><span style=\"color: #c00\">" + ev.toString().toHtmlEscaped() + "</span><span style=\"color: #444\">\"</span>";
	else if (ev.type() == QVariant::Int || ev.type() == QVariant::Double)
		s = "<span style=\"color: #00c\">" + ev.toString().toHtmlEscaped() + "</span>";
	else
		s = "<span style=\"color: #888\">unknown type</span>";
	m_consoleHistory.push_back(qMakePair(_js, s));
	s = "<html><body style=\"font-family: Monospace, Ubuntu Mono, Lucida Console, Courier New; margin: 0; font-size: 12pt\"><div style=\"position: absolute; bottom: 0; border: 0px; margin: 0px; width: 100%\">";
	for (auto const& i: m_consoleHistory)
		s +=	"<div style=\"border-bottom: 1 solid #eee; width: 100%\"><span style=\"float: left; width: 1em; color: #888; font-weight: bold\">&gt;</span><span style=\"color: #35d\">" + i.first.toHtmlEscaped() + "</span></div>"
				"<div style=\"border-bottom: 1 solid #eee; width: 100%\"><span style=\"float: left; width: 1em\">&nbsp;</span><span>" + i.second + "</span></div>";
	s += "</div></body></html>";
	ui->jsConsole->setHtml(s);
}

QString fromRaw(eth::h256 _n, unsigned* _inc = nullptr)
{
	if (_n)
	{
		std::string s((char const*)_n.data(), 32);
		auto l = s.find_first_of('\0');
		if (!l)
			return QString();
		if (l != string::npos)
		{
			auto p = s.find_first_not_of('\0', l);
			if (!(p == string::npos || (_inc && p == 31)))
				return QString();
			if (_inc)
				*_inc = (byte)s[31];
			s.resize(l);
		}
		for (auto i: s)
			if (i < 32)
				return QString();
		return QString::fromStdString(s);
	}
	return QString();
}

QString Main::pretty(eth::Address _a) const
{
	h256 n;

	if (h160 nameReg = (u160)state().storage(c_config, 0))
		n = state().storage(nameReg, (u160)(_a));

	if (!n)
		n = state().storage(m_nameReg, (u160)(_a));

	return fromRaw(n);
}

QString Main::render(eth::Address _a) const
{
	QString p = pretty(_a);
	if (!p.isNull())
		return p + " (" + QString::fromStdString(_a.abridged()) + ")";
	return QString::fromStdString(_a.abridged());
}

Address Main::fromString(QString const& _a) const
{
	if (_a == "(Create Contract)")
		return Address();

	string sn = _a.toStdString();
	if (sn.size() > 32)
		sn.resize(32);
	h256 n;
	memcpy(n.data(), sn.data(), sn.size());
	memset(n.data() + sn.size(), 0, 32 - sn.size());
	if (_a.size())
	{
		if (h160 nameReg = (u160)state().storage(c_config, 0))
			if (h256 a = state().storage(nameReg, n))
				return right160(a);

		if (h256 a = state().storage(m_nameReg, n))
			return right160(a);
	}
	if (_a.size() == 40)
		return Address(fromHex(_a.toStdString()));
	else
		return Address();
}

void Main::on_about_triggered()
{
    QMessageBox::about(this, "About AlethZero PoC-" + QString(eth::EthVersion).section('.', 1, 1), QString("AlethZero/v") + eth::EthVersion + "/" ETH_QUOTED(ETH_BUILD_TYPE) "/" ETH_QUOTED(ETH_BUILD_PLATFORM) "\n" ETH_QUOTED(ETH_COMMIT_HASH) + (ETH_CLEAN_REPO ? "\nCLEAN" : "\n+ LOCAL CHANGES") + "\n\nBy Gav Wood, 2014.\nBased on a design by Vitalik Buterin.\n\nTeam Ethereum++ includes: Eric Lombrozo, Marko Simovic, Alex Leverington, Tim Hughes and several others.");
}

void Main::on_paranoia_triggered()
{
	m_client->setParanoia(ui->paranoia->isChecked());
}

void Main::writeSettings()
{
	QSettings s("ethereum", "alethzero");
	QByteArray b;
	b.resize(sizeof(Secret) * m_myKeys.size());
	auto p = b.data();
	for (auto i: m_myKeys)
	{
		memcpy(p, &(i.secret()), sizeof(Secret));
		p += sizeof(Secret);
	}
	s.setValue("address", b);

	s.setValue("upnp", ui->upnp->isChecked());
	s.setValue("usePast", ui->usePast->isChecked());
	s.setValue("paranoia", ui->paranoia->isChecked());
	s.setValue("showAll", ui->showAll->isChecked());
	s.setValue("showAllAccounts", ui->showAllAccounts->isChecked());
	s.setValue("clientName", ui->clientName->text());
	s.setValue("idealPeers", ui->idealPeers->value());
	s.setValue("port", ui->port->value());
	s.setValue("url", ui->urlEdit->text());

	if (m_client->peerServer())
	{
		bytes d = m_client->peerServer()->savePeers();
		m_peers = QByteArray((char*)d.data(), (int)d.size());

	}
	s.setValue("peers", m_peers);
	s.setValue("nameReg", ui->nameReg->text());

	s.setValue("geometry", saveGeometry());
	s.setValue("windowState", saveState());
}

void Main::readSettings()
{
	QSettings s("ethereum", "alethzero");

	restoreGeometry(s.value("geometry").toByteArray());
	restoreState(s.value("windowState").toByteArray());

	m_myKeys.clear();
	QByteArray b = s.value("address").toByteArray();
	if (b.isEmpty())
		m_myKeys.append(KeyPair::create());
	else
	{
		h256 k;
		for (unsigned i = 0; i < b.size() / sizeof(Secret); ++i)
		{
			memcpy(&k, b.data() + i * sizeof(Secret), sizeof(Secret));
			if (!count(m_myKeys.begin(), m_myKeys.end(), KeyPair(k)))
				m_myKeys.append(KeyPair(k));
		}
	}
	m_client->setAddress(m_myKeys.back().address());
	m_peers = s.value("peers").toByteArray();
	ui->upnp->setChecked(s.value("upnp", true).toBool());
	ui->usePast->setChecked(s.value("usePast", true).toBool());
	ui->paranoia->setChecked(s.value("paranoia", false).toBool());
	ui->showAll->setChecked(s.value("showAll", false).toBool());
	ui->showAllAccounts->setChecked(s.value("showAllAccounts", false).toBool());
	ui->clientName->setText(s.value("clientName", "").toString());
	ui->idealPeers->setValue(s.value("idealPeers", ui->idealPeers->value()).toInt());
	ui->port->setValue(s.value("port", ui->port->value()).toInt());
	ui->nameReg->setText(s.value("NameReg", "").toString());
	ui->urlEdit->setText(s.value("url", "about:blank").toString());	//http://gavwood.com/gavcoin.html
	on_urlEdit_returnPressed();
}

void Main::on_importKey_triggered()
{
	QString s = QInputDialog::getText(this, "Import Account Key", "Enter account's secret key");
	bytes b = fromHex(s.toStdString());
	if (b.size() == 32)
	{
		auto k = KeyPair(h256(b));
		if (std::find(m_myKeys.begin(), m_myKeys.end(), k) == m_myKeys.end())
		{
			m_myKeys.append(k);
			m_keysChanged = true;
			update();
		}
		else
			QMessageBox::warning(this, "Already Have Key", "Could not import the secret key: we already own this account.");
	}
	else
		QMessageBox::warning(this, "Invalid Entry", "Could not import the secret key; invalid key entered. Make sure it is 64 hex characters (0-9 or A-F).");
}

void Main::on_exportKey_triggered()
{
	if (ui->ourAccounts->currentRow() >= 0 && ui->ourAccounts->currentRow() < m_myKeys.size())
	{
		auto k = m_myKeys[ui->ourAccounts->currentRow()];
		QMessageBox::information(this, "Export Account Key", "Secret key to account " + render(k.address()) + " is:\n" + QString::fromStdString(toHex(k.sec().ref())));
	}
}

void Main::on_urlEdit_returnPressed()
{
	ui->webView->setUrl(ui->urlEdit->text());
}

void Main::on_nameReg_textChanged()
{
	string s = ui->nameReg->text().toStdString();
	if (s.size() == 40)
	{
		m_nameReg = Address(fromHex(s));
		refresh(true);
	}
	else
		m_nameReg = Address();
}

void Main::refreshMining()
{
	eth::ClientGuard g(m_client.get());
	eth::MineProgress p = m_client->miningProgress();
	ui->mineStatus->setText(m_client->isMining() ? QString("%1s @ %2kH/s").arg(p.ms / 1000).arg(p.ms ? p.hashes / p.ms : 0) : "Not mining");
	if (!ui->miningView->isVisible())
		return;
	list<eth::MineInfo> l = m_client->miningHistory();
	static uint lh = 0;
	if (p.hashes < lh)
		ui->miningView->resetStats();
	lh = p.hashes;
	ui->miningView->appendStats(l, p);
/*	if (p.ms)
		for (eth::MineInfo const& i: l)
			cnote << i.hashes * 10 << "h/sec, need:" << i.requirement << " best:" << i.best << " best-so-far:" << p.best << " avg-speed:" << (p.hashes * 1000 / p.ms) << "h/sec";
*/
}

void Main::refreshNetwork()
{
	auto ps = m_client->peers();

	ui->peerCount->setText(QString::fromStdString(toString(ps.size())) + " peer(s)");
	ui->peers->clear();
	for (PeerInfo const& i: ps)
		ui->peers->addItem(QString("%3 ms - %1:%2 - %4").arg(i.host.c_str()).arg(i.port).arg(chrono::duration_cast<chrono::milliseconds>(i.lastPing).count()).arg(i.clientVersion.c_str()));
}

eth::State const& Main::state() const
{
	return ui->preview->isChecked() ? m_client->postState() : m_client->state();
}

void Main::updateBlockCount()
{
	auto d = m_client->blockChain().details();
	auto diff = BlockInfo(m_client->blockChain().block()).difficulty;
	ui->blockCount->setText(QString("#%1 @%3 T%2 N%4").arg(d.number).arg(toLog2(d.totalDifficulty)).arg(toLog2(diff)).arg(eth::c_protocolVersion));
}

void Main::on_blockChainFilter_textChanged()
{
	static QTimer* s_delayed = nullptr;
	if (!s_delayed)
	{
		s_delayed = new QTimer(this);
		s_delayed->setSingleShot(true);
		connect(s_delayed, SIGNAL(timeout()), SLOT(refreshBlockChain()));
	}
	s_delayed->stop();
	s_delayed->start(200);
}

static bool blockMatch(string const& _f, eth::BlockDetails const& _b, h256 _h, BlockChain const& _bc)
{
	try
	{
		if (_f.size() > 1 && _f.size() < 10 && _f[0] == '#' && stoul(_f.substr(1)) == _b.number)
			return true;
	}
	catch (...) {}
	if (toHex(_h.ref()).find(_f) != string::npos)
		return true;
	BlockInfo bi(_bc.block(_h));
	string info = toHex(bi.stateRoot.ref()) + " " + toHex(bi.coinbaseAddress.ref()) + " " + toHex(bi.transactionsRoot.ref()) + " " + toHex(bi.sha3Uncles.ref());
	if (info.find(_f) != string::npos)
		return true;
	return false;
}

static bool transactionMatch(string const& _f, Transaction const& _t)
{
	string info = toHex(_t.receiveAddress.ref()) + " " + toHex(_t.sha3(true).ref()) + " " + toHex(_t.sha3(false).ref()) + " " + toHex(_t.sender().ref());
	if (info.find(_f) != string::npos)
		return true;
	return false;
}

void Main::refreshBlockChain()
{
	eth::ClientGuard g(m_client.get());
	auto const& st = state();

	QByteArray oldSelected = ui->blocks->count() ? ui->blocks->currentItem()->data(Qt::UserRole).toByteArray() : QByteArray();
	ui->blocks->clear();

	string filter = ui->blockChainFilter->text().toLower().toStdString();
	auto const& bc = m_client->blockChain();
	unsigned i = (ui->showAll->isChecked() || !filter.empty()) ? (unsigned)-1 : 10;
	for (auto h = bc.currentHash(); h != bc.genesisHash() && i; h = bc.details(h).parent, --i)
	{
		auto d = bc.details(h);
		auto bm = blockMatch(filter, d, h, bc);
		if (bm)
		{
			QListWidgetItem* blockItem = new QListWidgetItem(QString("#%1 %2").arg(d.number).arg(h.abridged().c_str()), ui->blocks);
			auto hba = QByteArray((char const*)h.data(), h.size);
			blockItem->setData(Qt::UserRole, hba);
			if (oldSelected == hba)
				blockItem->setSelected(true);
		}
		int n = 0;
		auto b = bc.block(h);
		for (auto const& i: RLP(b)[1])
		{
			Transaction t(i[0].data());
			if (bm || transactionMatch(filter, t))
			{
				QString s = t.receiveAddress ?
					QString("    %2 %5> %3: %1 [%4]")
						.arg(formatBalance(t.value).c_str())
						.arg(render(t.safeSender()))
						.arg(render(t.receiveAddress))
						.arg((unsigned)t.nonce)
						.arg(st.addressHasCode(t.receiveAddress) ? '*' : '-') :
					QString("    %2 +> %3: %1 [%4]")
						.arg(formatBalance(t.value).c_str())
						.arg(render(t.safeSender()))
						.arg(render(right160(sha3(rlpList(t.safeSender(), t.nonce)))))
						.arg((unsigned)t.nonce);
				QListWidgetItem* txItem = new QListWidgetItem(s, ui->blocks);
				auto hba = QByteArray((char const*)h.data(), h.size);
				txItem->setData(Qt::UserRole, hba);
				txItem->setData(Qt::UserRole + 1, n);
				if (oldSelected == hba)
					txItem->setSelected(true);
			}
			n++;
		}
	}

	if (!ui->blocks->currentItem())
		ui->blocks->setCurrentRow(0);
}

void Main::refresh(bool _override)
{
	eth::ClientGuard g(m_client.get());
	auto const& st = state();

	bool c = m_client->changed();
	if (c || _override)
	{
		changed();

		updateBlockCount();

		auto acs = st.addresses();
		ui->accounts->clear();
		ui->contracts->clear();
		for (auto n = 0; n < 2; ++n)
			for (auto i: acs)
			{
				auto r = render(i.first);
				if (r.contains('(') == !n)
				{
					if (n == 0 || ui->showAllAccounts->isChecked())
						(new QListWidgetItem(QString("%2: %1 [%3]").arg(formatBalance(i.second).c_str()).arg(r).arg((unsigned)state().transactionsFrom(i.first)), ui->accounts))
							->setData(Qt::UserRole, QByteArray((char const*)i.first.data(), Address::size));
					if (st.addressHasCode(i.first))
						(new QListWidgetItem(QString("%2: %1 [%3]").arg(formatBalance(i.second).c_str()).arg(r).arg((unsigned)st.transactionsFrom(i.first)), ui->contracts))
							->setData(Qt::UserRole, QByteArray((char const*)i.first.data(), Address::size));

					if (r.contains('('))
					{
						// A namereg address
						QString s = pretty(i.first);
						if (ui->destination->findText(s, Qt::MatchExactly | Qt::MatchCaseSensitive) == -1)
							ui->destination->addItem(s);
					}
				}
			}

		for (int i = 0; i < ui->destination->count(); ++i)
			if (ui->destination->itemText(i) != "(Create Contract)" && !fromString(ui->destination->itemText(i)))
				ui->destination->removeItem(i--);

		ui->transactionQueue->clear();
		for (Transaction const& t: m_client->pending())
		{
			QString s = t.receiveAddress ?
				QString("%2 %5> %3: %1 [%4]")
					.arg(formatBalance(t.value).c_str())
					.arg(render(t.safeSender()))
					.arg(render(t.receiveAddress))
					.arg((unsigned)t.nonce)
					.arg(st.addressHasCode(t.receiveAddress) ? '*' : '-') :
				QString("%2 +> %3: %1 [%4]")
					.arg(formatBalance(t.value).c_str())
					.arg(render(t.safeSender()))
					.arg(render(right160(sha3(rlpList(t.safeSender(), t.nonce)))))
					.arg((unsigned)t.nonce);
			ui->transactionQueue->addItem(s);
		}

		refreshBlockChain();
	}

	if (c || m_keysChanged || _override)
	{
		m_keysChanged = false;
		ui->ourAccounts->clear();
		u256 totalBalance = 0;
		map<Address, pair<QString, u256>> altCoins;
		Address coinsAddr = right160(st.storage(c_config, 1));
		for (unsigned i = 0; i < st.storage(coinsAddr, 0); ++i)
			altCoins[right160(st.storage(coinsAddr, st.storage(coinsAddr, i + 1)))] = make_pair(fromRaw(st.storage(coinsAddr, i + 1)), 0);
//		u256 totalGavCoinBalance = 0;
		for (auto i: m_myKeys)
		{
			u256 b = st.balance(i.address());
			(new QListWidgetItem(QString("%2: %1 [%3]").arg(formatBalance(b).c_str()).arg(render(i.address())).arg((unsigned)st.transactionsFrom(i.address())), ui->ourAccounts))
				->setData(Qt::UserRole, QByteArray((char const*)i.address().data(), Address::size));
			totalBalance += b;

			for (auto& c: altCoins)
				c.second.second += (u256)st.storage(c.first, (u160)i.address());
		}

		QString b;
		for (auto const& c: altCoins)
			if (c.second.second)
				b += QString::fromStdString(toString(c.second.second)) + " " + c.second.first.toUpper() + " | ";
		ui->balance->setText(b + QString::fromStdString(formatBalance(totalBalance)));
	}
}

string Main::renderDiff(eth::StateDiff const& _d) const
{
	stringstream s;

	auto indent = "<code style=\"white-space: pre\">     </code>";
	for (auto const& i: _d.accounts)
	{
		s << "<hr/>";

		eth::AccountDiff const& ad = i.second;
		s << "<code style=\"white-space: pre; font-weight: bold\">" << ad.lead() << "  </code>" << " <b>" << render(i.first).toStdString() << "</b>";
		if (!ad.exist.to())
			continue;

		if (ad.balance)
		{
			s << "<br/>" << indent << "Balance " << std::dec << formatBalance(ad.balance.to());
			s << " <b>" << std::showpos << (((eth::bigint)ad.balance.to()) - ((eth::bigint)ad.balance.from())) << std::noshowpos << "</b>";
		}
		if (ad.nonce)
		{
			s << "<br/>" << indent << "Count #" << std::dec << ad.nonce.to();
			s << " <b>" << std::showpos << (((eth::bigint)ad.nonce.to()) - ((eth::bigint)ad.nonce.from())) << std::noshowpos << "</b>";
		}
		if (ad.code)
		{
			s << "<br/>" << indent << "Code " << std::hex << ad.code.to().size() << " bytes";
			if (ad.code.from().size())
				 s << " (" << ad.code.from().size() << " bytes)";
		}

		for (pair<u256, eth::Diff<u256>> const& i: ad.storage)
		{
			s << "<br/><code style=\"white-space: pre\">";
			if (!i.second.from())
				s << " + ";
			else if (!i.second.to())
				s << "XXX";
			else
				s << " * ";
			s << "  </code>";

			s << prettyU256(i.first).toStdString();
/*			if (i.first > u256(1) << 246)
				s << (h256)i.first;
			else if (i.first > u160(1) << 150)
				s << (h160)(u160)i.first;
			else
				s << std::hex << i.first;
*/
			if (!i.second.from())
				s << ": " << prettyU256(i.second.to()).toStdString();
			else if (!i.second.to())
				s << " (" << prettyU256(i.second.from()).toStdString() << ")";
			else
				s << ": " << prettyU256(i.second.to()).toStdString() << " (" << prettyU256(i.second.from()).toStdString() << ")";
		}
	}
	return s.str();
}

void Main::on_transactionQueue_currentItemChanged()
{
	ui->pendingInfo->clear();
	eth::ClientGuard g(m_client.get());

	stringstream s;
	int i = ui->transactionQueue->currentRow();
	if (i >= 0)
	{
		Transaction tx(m_client->postState().pending()[i]);
		auto ss = tx.safeSender();
		h256 th = sha3(rlpList(ss, tx.nonce));
		s << "<h3>" << th << "</h3>";
		s << "From: <b>" << pretty(ss).toStdString() << "</b> " << ss;
		if (tx.isCreation())
			s << "<br/>Creates: <b>" << pretty(right160(th)).toStdString() << "</b> " << right160(th);
		else
			s << "<br/>To: <b>" << pretty(tx.receiveAddress).toStdString() << "</b> " << tx.receiveAddress;
		s << "<br/>Value: <b>" << formatBalance(tx.value) << "</b>";
		s << "&nbsp;&emsp;&nbsp;#<b>" << tx.nonce << "</b>";
		s << "<br/>Gas price: <b>" << formatBalance(tx.gasPrice) << "</b>";
		s << "<br/>Gas: <b>" << tx.gas << "</b>";
		if (tx.isCreation())
		{
			if (tx.data.size())
				s << "<h4>Code</h4>" << disassemble(tx.data);
		}
		else
		{
			if (tx.data.size())
				s << eth::memDump(tx.data, 16, true);
		}
		s << "<hr/>";

//		s << "Pre: " << fs.rootHash() << "<br/>";
//		s << "Post: <b>" << ts.rootHash() << "</b>";

		s << renderDiff(m_client->postState().pendingDiff(i));
	}

	ui->pendingInfo->setHtml(QString::fromStdString(s.str()));
}

void Main::ourAccountsRowsMoved()
{
	QList<KeyPair> myKeys;
	for (int i = 0; i < ui->ourAccounts->count(); ++i)
	{
		auto hba = ui->ourAccounts->item(i)->data(Qt::UserRole).toByteArray();
		auto h = Address((byte const*)hba.data(), Address::ConstructFromPointer);
		for (auto i: m_myKeys)
			if (i.address() == h)
				myKeys.push_back(i);
	}
	m_myKeys = myKeys;
	if (m_ethereum)
		m_ethereum->setAccounts(myKeys);
}

void Main::on_inject_triggered()
{
	QString s = QInputDialog::getText(this, "Inject Transaction", "Enter transaction dump in hex");
	bytes b = fromHex(s.toStdString());
	m_client->inject(&b);
	refresh();
}

void Main::on_blocks_currentItemChanged()
{
	ui->info->clear();
	ui->debugCurrent->setEnabled(false);
	ui->debugDumpState->setEnabled(false);
	ui->debugDumpStatePre->setEnabled(false);
	eth::ClientGuard g(m_client.get());
	if (auto item = ui->blocks->currentItem())
	{
		auto hba = item->data(Qt::UserRole).toByteArray();
		assert(hba.size() == 32);
		auto h = h256((byte const*)hba.data(), h256::ConstructFromPointer);
		auto details = m_client->blockChain().details(h);
		auto blockData = m_client->blockChain().block(h);
		auto block = RLP(blockData);
		BlockInfo info(blockData);

		stringstream s;

		if (item->data(Qt::UserRole + 1).isNull())
		{
			char timestamp[64];
			time_t rawTime = (time_t)(uint64_t)info.timestamp;
			strftime(timestamp, 64, "%c", localtime(&rawTime));
			s << "<h3>" << h << "</h3>";
			s << "<h4>#" << info.number;
			s << "&nbsp;&emsp;&nbsp;<b>" << timestamp << "</b></h4>";
			s << "<br/>D/TD: <b>2^" << log2((double)info.difficulty) << "</b>/<b>2^" << log2((double)details.totalDifficulty) << "</b>";
			s << "&nbsp;&emsp;&nbsp;Children: <b>" << details.children.size() << "</b></h5>";
			s << "<br/>Gas used/limit: <b>" << info.gasUsed << "</b>/<b>" << info.gasLimit << "</b>";
			s << "&nbsp;&emsp;&nbsp;Minimum gas price: <b>" << formatBalance(info.minGasPrice) << "</b>";
			s << "<br/>Coinbase: <b>" << pretty(info.coinbaseAddress).toHtmlEscaped().toStdString() << "</b> " << info.coinbaseAddress;
			s << "<br/>Nonce: <b>" << info.nonce << "</b>";
			s << "<br/>Parent: <b>" << info.parentHash << "</b>";
			s << "<br/>Transactions: <b>" << block[1].itemCount() << "</b> @<b>" << info.transactionsRoot << "</b>";
			s << "<br/>Uncles: <b>" << block[2].itemCount() << "</b> @<b>" << info.sha3Uncles << "</b>";
			s << "<br/>Pre: <b>" << BlockInfo(m_client->blockChain().block(info.parentHash)).stateRoot << "</b>";
			for (auto const& i: block[1])
				s << "<br/>" << sha3(i[0].data()).abridged() << ": <b>" << i[1].toHash<h256>() << "</b> [<b>" << i[2].toInt<u256>() << "</b> used]";
			s << "<br/>Post: <b>" << info.stateRoot << "</b>";
		}
		else
		{
			unsigned txi = item->data(Qt::UserRole + 1).toInt();
			Transaction tx(block[1][txi][0].data());
			auto ss = tx.safeSender();
			h256 th = sha3(rlpList(ss, tx.nonce));
			s << "<h3>" << th << "</h3>";
			s << "<h4>" << h << "[<b>" << txi << "</b>]</h4>";
			s << "<br/>From: <b>" << pretty(ss).toHtmlEscaped().toStdString() << "</b> " << ss;
			if (tx.isCreation())
				s << "<br/>Creates: <b>" << pretty(right160(th)).toHtmlEscaped().toStdString() << "</b> " << right160(th);
			else
				s << "<br/>To: <b>" << pretty(tx.receiveAddress).toHtmlEscaped().toStdString() << "</b> " << tx.receiveAddress;
			s << "<br/>Value: <b>" << formatBalance(tx.value) << "</b>";
			s << "&nbsp;&emsp;&nbsp;#<b>" << tx.nonce << "</b>";
			s << "<br/>Gas price: <b>" << formatBalance(tx.gasPrice) << "</b>";
			s << "<br/>Gas: <b>" << tx.gas << "</b>";
			s << "<br/>V: <b>" << hex << nouppercase << (int)tx.vrs.v << "</b>";
			s << "<br/>R: <b>" << hex << nouppercase << tx.vrs.r << "</b>";
			s << "<br/>S: <b>" << hex << nouppercase << tx.vrs.s << "</b>";
			s << "<br/>Msg: <b>" << tx.sha3(false) << "</b>";
			if (tx.isCreation())
			{
				if (tx.data.size())
					s << "<h4>Code</h4>" << disassemble(tx.data);
			}
			else
			{
				if (tx.data.size())
					s << eth::memDump(tx.data, 16, true);
			}
			auto st = eth::State(m_client->state().db(), m_client->blockChain(), h);
			eth::State before = st.fromPending(txi);
			eth::State after = st.fromPending(txi + 1);
			s << renderDiff(before.diff(after));
			ui->debugCurrent->setEnabled(true);
			ui->debugDumpState->setEnabled(true);
			ui->debugDumpStatePre->setEnabled(true);
		}

		ui->info->appendHtml(QString::fromStdString(s.str()));
	}
}

void Main::on_debugCurrent_triggered()
{
	eth::ClientGuard g(m_client.get());
	if (auto item = ui->blocks->currentItem())
	{
		auto hba = item->data(Qt::UserRole).toByteArray();
		assert(hba.size() == 32);
		auto h = h256((byte const*)hba.data(), h256::ConstructFromPointer);

		if (!item->data(Qt::UserRole + 1).isNull())
		{
			eth::State st(m_client->state().db(), m_client->blockChain(), h);
			unsigned txi = item->data(Qt::UserRole + 1).toInt();
			m_executiveState = st.fromPending(txi);
			m_currentExecution = unique_ptr<Executive>(new Executive(m_executiveState));
			Transaction t = st.pending()[txi];
			auto r = t.rlp();
			populateDebugger(&r);
			m_currentExecution.reset();
		}
	}
}

void Main::on_debugDumpState_triggered(int _add)
{
	eth::ClientGuard g(m_client.get());
	if (auto item = ui->blocks->currentItem())
	{
		auto hba = item->data(Qt::UserRole).toByteArray();
		assert(hba.size() == 32);
		auto h = h256((byte const*)hba.data(), h256::ConstructFromPointer);

		if (!item->data(Qt::UserRole + 1).isNull())
		{
			QString fn = QFileDialog::getSaveFileName(this, "Select file to output state dump");
			ofstream f(fn.toStdString());
			if (f.is_open())
			{
				eth::State st(m_client->state().db(), m_client->blockChain(), h);
				unsigned txi = item->data(Qt::UserRole + 1).toInt();
				f << st.fromPending(txi + _add) << endl;
			}
		}
	}
}

void Main::on_debugDumpStatePre_triggered()
{
	on_debugDumpState_triggered(0);
}

void Main::populateDebugger(eth::bytesConstRef _r)
{
	bool done = m_currentExecution->setup(_r);
	if (!done)
	{
		debugFinished();
		vector<WorldState const*> levels;
		m_codes.clear();
		bytesConstRef lastExtCode;
		bytesConstRef lastData;
		h256 lastHash;
		h256 lastDataHash;
		auto onOp = [&](uint64_t steps, Instruction inst, unsigned newMemSize, eth::bigint gasCost, void* voidVM, void const* voidExt)
		{
			eth::VM& vm = *(eth::VM*)voidVM;
			eth::ExtVM const& ext = *(eth::ExtVM const*)voidExt;
			if (ext.code != lastExtCode)
			{
				lastExtCode = ext.code;
				lastHash = sha3(lastExtCode);
				if (!m_codes.count(lastHash))
					m_codes[lastHash] = ext.code.toBytes();
			}
			if (ext.data != lastData)
			{
				lastData = ext.data;
				lastDataHash = sha3(lastData);
				if (!m_codes.count(lastDataHash))
					m_codes[lastDataHash] = ext.data.toBytes();
			}
			if (levels.size() < ext.level)
				levels.push_back(&m_history.back());
			else
				levels.resize(ext.level);
			m_history.append(WorldState({steps, ext.myAddress, vm.curPC(), inst, newMemSize, vm.gas(), lastHash, lastDataHash, vm.stack(), vm.memory(), gasCost, ext.state().storage(ext.myAddress), levels}));
		};
		m_currentExecution->go(onOp);
		initDebugger();
		updateDebugger();
	}
}

void Main::on_contracts_currentItemChanged()
{
	ui->contractInfo->clear();
	eth::ClientGuard l(&*m_client);
	if (auto item = ui->contracts->currentItem())
	{
		auto hba = item->data(Qt::UserRole).toByteArray();
		assert(hba.size() == 20);
		auto h = h160((byte const*)hba.data(), h160::ConstructFromPointer);

		stringstream s;
		try
		{
			auto storage = state().storage(h);
			for (auto const& i: storage)
				s << "@" << showbase << hex << prettyU256(i.first).toStdString() << "&nbsp;&nbsp;&nbsp;&nbsp;" << showbase << hex << prettyU256(i.second).toStdString() << "<br/>";
			s << "<h4>Body Code</h4>" << disassemble(state().code(h));
			ui->contractInfo->appendHtml(QString::fromStdString(s.str()));
		}
		catch (eth::InvalidTrie)
		{
			ui->contractInfo->appendHtml("Corrupted trie.");
		}
	}
}

void Main::on_idealPeers_valueChanged()
{
	if (m_client->peerServer())
		m_client->peerServer()->setIdealPeerCount(ui->idealPeers->value());
}

void Main::on_ourAccounts_doubleClicked()
{
	auto hba = ui->ourAccounts->currentItem()->data(Qt::UserRole).toByteArray();
	auto h = Address((byte const*)hba.data(), Address::ConstructFromPointer);
	qApp->clipboard()->setText(QString::fromStdString(toHex(h.asArray())));
}

void Main::on_log_doubleClicked()
{
	qApp->clipboard()->setText(ui->log->currentItem()->text());
}

void Main::on_accounts_doubleClicked()
{
	auto hba = ui->accounts->currentItem()->data(Qt::UserRole).toByteArray();
	auto h = Address((byte const*)hba.data(), Address::ConstructFromPointer);
	qApp->clipboard()->setText(QString::fromStdString(toHex(h.asArray())));
}

void Main::on_contracts_doubleClicked()
{
	auto hba = ui->contracts->currentItem()->data(Qt::UserRole).toByteArray();
	auto h = Address((byte const*)hba.data(), Address::ConstructFromPointer);
	qApp->clipboard()->setText(QString::fromStdString(toHex(h.asArray())));
}

void Main::on_destination_currentTextChanged()
{
	if (ui->destination->currentText().size() && ui->destination->currentText() != "(Create Contract)")
		if (Address a = fromString(ui->destination->currentText()))
			ui->calculatedName->setText(render(a));
		else
			ui->calculatedName->setText("Unknown Address");
	else
		ui->calculatedName->setText("Create Contract");
	on_data_textChanged();
//	updateFee();
}

void Main::on_data_textChanged()
{
	m_pcWarp.clear();
	if (isCreation())
	{
		string src = ui->data->toPlainText().toStdString();
		vector<string> errors;
		QString lll;
		if (src.find_first_not_of("1234567890abcdefABCDEF") == string::npos && src.size() % 2 == 0)
		{
			m_data = fromHex(src);
		}
		else
		{
			auto asmcode = eth::compileLLLToAsm(src, false);
			auto asmcodeopt = eth::compileLLLToAsm(ui->data->toPlainText().toStdString(), true);
			m_data = eth::compileLLL(ui->data->toPlainText().toStdString(), true, &errors);
			if (errors.size())
			{
				try
				{
					m_data = compile(src);
					for (auto& i: errors)
						i = "(LLL " + i + ")";
				}
				catch (string err)
				{
					errors.push_back("Serpent " + err);
				}
			}
			else
				lll = "<h4>Opt</h4><pre>" + QString::fromStdString(asmcodeopt).toHtmlEscaped() + "</pre><h4>Pre</h4><pre>" + QString::fromStdString(asmcode).toHtmlEscaped() + "</pre>";
		}
		QString errs;
		if (errors.size())
		{
			errs = "<h4>Errors</h4>";
			for (auto const& i: errors)
				errs.append("<div style=\"border-left: 6px solid #c00; margin-top: 2px\">" + QString::fromStdString(i).toHtmlEscaped() + "</div>");
		}
		ui->code->setHtml(errs + lll + "<h4>Code</h4>" + QString::fromStdString(disassemble(m_data)).toHtmlEscaped());
		ui->gas->setMinimum((qint64)state().createGas(m_data.size(), 0));
		if (!ui->gas->isEnabled())
			ui->gas->setValue(m_backupGas);
		ui->gas->setEnabled(true);
	}
	else
	{
		m_data.clear();
		QString s = ui->data->toPlainText();
		while (s.size())
		{
			QRegExp r("(@|\\$)?\"([^\"]*)\"(.*)");
			QRegExp h("(@|\\$)?(0x)?(([a-fA-F0-9])+)(.*)");
			if (r.exactMatch(s))
			{
				for (auto i: r.cap(2))
					m_data.push_back((byte)i.toLatin1());
				if (r.cap(1) != "$")
					for (int i = r.cap(2).size(); i < 32; ++i)
						m_data.push_back(0);
				else
					m_data.push_back(0);
				s = r.cap(3);
			}
			else if (h.exactMatch(s))
			{
				bytes bs = fromHex((((h.cap(3).size() & 1) ? "0" : "") + h.cap(3)).toStdString());
				if (h.cap(1) != "$")
					for (auto i = bs.size(); i < 32; ++i)
						m_data.push_back(0);
				for (auto b: bs)
					m_data.push_back(b);
				s = h.cap(5);
			}
			else
				s = s.mid(1);
		}
		ui->code->setHtml(QString::fromStdString(eth::memDump(m_data, 8, true)));
		if (m_client->postState().addressHasCode(fromString(ui->destination->currentText())))
		{
			ui->gas->setMinimum((qint64)state().callGas(m_data.size(), 1));
			if (!ui->gas->isEnabled())
				ui->gas->setValue(m_backupGas);
			ui->gas->setEnabled(true);
		}
		else
		{
			if (ui->gas->isEnabled())
				m_backupGas = ui->gas->value();
			ui->gas->setValue((qint64)state().callGas(m_data.size()));
			ui->gas->setEnabled(false);
		}
	}
	updateFee();
}

void Main::on_killBlockchain_triggered()
{
	writeSettings();
	ui->mine->setChecked(false);
	ui->net->setChecked(false);
	m_client.reset();
	m_client.reset(new Client("AlethZero", Address(), string(), true));
	readSettings();
}

bool Main::isCreation() const
{
	return ui->destination->currentText().isEmpty() || ui->destination->currentText() == "(Create Contract)";
}

u256 Main::fee() const
{
	return ui->gas->value() * gasPrice();
}

u256 Main::value() const
{
	if (ui->valueUnits->currentIndex() == -1)
		return 0;
	return ui->value->value() * units()[units().size() - 1 - ui->valueUnits->currentIndex()].first;
}

u256 Main::gasPrice() const
{
	if (ui->gasPriceUnits->currentIndex() == -1)
		return 0;
	return ui->gasPrice->value() * units()[units().size() - 1 - ui->gasPriceUnits->currentIndex()].first;
}

u256 Main::total() const
{
	return value() + fee();
}

void Main::updateFee()
{
	ui->fee->setText(QString("(gas sub-total: %1)").arg(formatBalance(fee()).c_str()));
	auto totalReq = total();
	ui->total->setText(QString("Total: %1").arg(formatBalance(totalReq).c_str()));

	bool ok = false;
	for (auto i: m_myKeys)
		if (state().balance(i.address()) >= totalReq)
		{
			ok = true;
			break;
		}
	ui->send->setEnabled(ok);
	QPalette p = ui->total->palette();
	p.setColor(QPalette::WindowText, QColor(ok ? 0x00 : 0x80, 0x00, 0x00));
	ui->total->setPalette(p);
}

void Main::on_net_triggered()
{
	ui->port->setEnabled(!ui->net->isChecked());
	ui->clientName->setEnabled(!ui->net->isChecked());
    string n = string("AlethZero/v") + eth::EthVersion;
	if (ui->clientName->text().size())
		n += "/" + ui->clientName->text().toStdString();
	n +=  "/" ETH_QUOTED(ETH_BUILD_TYPE) "/" ETH_QUOTED(ETH_BUILD_PLATFORM);
	m_client->setClientVersion(n);
	if (ui->net->isChecked())
	{
		m_client->startNetwork(ui->port->value(), string(), 0, NodeMode::Full, ui->idealPeers->value(), std::string(), ui->upnp->isChecked());
		if (m_peers.size() && ui->usePast->isChecked())
			m_client->peerServer()->restorePeers(bytesConstRef((byte*)m_peers.data(), m_peers.size()));
	}
	else
		m_client->stopNetwork();
}

void Main::on_connect_triggered()
{
	if (!ui->net->isChecked())
	{
		ui->net->setChecked(true);
		on_net_triggered();
	}
	bool ok = false;
	QString s = QInputDialog::getItem(this, "Connect to a Network Peer", "Enter a peer to which a connection may be made:", m_servers, m_servers.count() ? rand() % m_servers.count() : 0, true, &ok);
	if (ok && s.contains(":"))
	{
		string host = s.section(":", 0, 0).toStdString();
		unsigned short port = s.section(":", 1).toInt();
		m_client->connect(host, port);
	}
}

void Main::on_verbosity_valueChanged()
{
	g_logVerbosity = ui->verbosity->value();
	ui->verbosityLabel->setText(QString::number(g_logVerbosity));
}

void Main::on_mine_triggered()
{
	if (ui->mine->isChecked())
	{
		m_client->setAddress(m_myKeys.last().address());
		m_client->startMining();
	}
	else
		m_client->stopMining();
}

void Main::on_send_clicked()
{
	u256 totalReq = value() + fee();
	eth::ClientGuard l(&*m_client);
	for (auto i: m_myKeys)
		if (m_client->postState().balance(i.address()) >= totalReq)
		{
			debugFinished();
			Secret s = i.secret();
			if (isCreation())
				m_client->transact(s, value(), m_data, ui->gas->value(), gasPrice());
			else
				m_client->transact(s, value(), fromString(ui->destination->currentText()), m_data, ui->gas->value(), gasPrice());
			refresh();
			return;
		}
	statusBar()->showMessage("Couldn't make transaction: no single account contains at least the required amount.");
}

void Main::on_debug_clicked()
{
	debugFinished();
	try
	{
		u256 totalReq = value() + fee();
		eth::ClientGuard l(&*m_client);
		for (auto i: m_myKeys)
			if (m_client->state().balance(i.address()) >= totalReq)
			{
				Secret s = i.secret();
				m_executiveState = state();
				m_currentExecution = unique_ptr<Executive>(new Executive(m_executiveState));
				Transaction t;
				t.nonce = m_executiveState.transactionsFrom(toAddress(s));
				t.value = value();
				t.gasPrice = gasPrice();
				t.gas = ui->gas->value();
				t.data = m_data;
				t.receiveAddress = isCreation() ? Address() : fromString(ui->destination->currentText());
				t.sign(s);
				auto r = t.rlp();
				populateDebugger(&r);
				m_currentExecution.reset();
				return;
			}
		statusBar()->showMessage("Couldn't make transaction: no single account contains at least the required amount.");
	}
	catch (eth::Exception const& _e)
	{
		statusBar()->showMessage("Error running transaction: " + QString::fromStdString(_e.description()));
	}
}

void Main::on_create_triggered()
{
	m_myKeys.append(KeyPair::create());
	m_keysChanged = true;
}

void Main::on_debugStep_triggered()
{
	auto l = m_history[ui->debugTimeline->value()].levels.size();
	if (ui->debugTimeline->value() < m_history.size() && m_history[ui->debugTimeline->value() + 1].levels.size() > l)
	{
		on_debugStepInto_triggered();
		if (m_history[ui->debugTimeline->value()].levels.size() > l)
			on_debugStepOut_triggered();
	}
	else
		on_debugStepInto_triggered();
}

void Main::on_debugStepInto_triggered()
{
	ui->debugTimeline->setValue(ui->debugTimeline->value() + 1);
	ui->callStack->setCurrentRow(0);
}

void Main::on_debugStepOut_triggered()
{
	if (ui->debugTimeline->value() < m_history.size())
	{
		auto ls = m_history[ui->debugTimeline->value()].levels.size();
		auto l = ui->debugTimeline->value();
		for (; l < m_history.size() && m_history[l].levels.size() >= ls; ++l) {}
		ui->debugTimeline->setValue(l);
		ui->callStack->setCurrentRow(0);
	}
}

void Main::on_debugStepBackInto_triggered()
{
	ui->debugTimeline->setValue(ui->debugTimeline->value() - 1);
	ui->callStack->setCurrentRow(0);
}

void Main::on_debugStepBack_triggered()
{
	auto l = m_history[ui->debugTimeline->value()].levels.size();
	if (ui->debugTimeline->value() > 0 && m_history[ui->debugTimeline->value() - 1].levels.size() > l)
	{
		on_debugStepBackInto_triggered();
		if (m_history[ui->debugTimeline->value()].levels.size() > l)
			on_debugStepBackOut_triggered();
	}
	else
		on_debugStepBackInto_triggered();
}

void Main::on_debugStepBackOut_triggered()
{
	if (ui->debugTimeline->value() > 0)
	{
		auto ls = m_history[ui->debugTimeline->value()].levels.size();
		int l = ui->debugTimeline->value();
		for (; l > 0 && m_history[l].levels.size() >= ls; --l) {}
		ui->debugTimeline->setValue(l);
		ui->callStack->setCurrentRow(0);
	}
}

void Main::on_dumpTrace_triggered()
{
	QString fn = QFileDialog::getSaveFileName(this, "Select file to output EVM trace");
	ofstream f(fn.toStdString());
	if (f.is_open())
		for (WorldState const& ws: m_history)
			f << ws.cur << " " << hex << toHex(eth::toCompactBigEndian(ws.curPC, 1)) << " " << hex << toHex(eth::toCompactBigEndian((int)(byte)ws.inst, 1)) << " " << hex << toHex(eth::toCompactBigEndian((uint64_t)ws.gas, 1)) << endl;
}

void Main::on_dumpTraceStorage_triggered()
{
	QString fn = QFileDialog::getSaveFileName(this, "Select file to output EVM trace");
	ofstream f(fn.toStdString());
	if (f.is_open())
		for (WorldState const& ws: m_history)
		{
			if (ws.inst == Instruction::STOP || ws.inst == Instruction::RETURN || ws.inst == Instruction::SUICIDE)
				for (auto i: ws.storage)
					f << toHex(eth::toCompactBigEndian(i.first, 1)) << " " << toHex(eth::toCompactBigEndian(i.second, 1)) << endl;
			f << ws.cur << " " << hex << toHex(eth::toCompactBigEndian(ws.curPC, 1)) << " " << hex << toHex(eth::toCompactBigEndian((int)(byte)ws.inst, 1)) << " " << hex << toHex(eth::toCompactBigEndian((uint64_t)ws.gas, 1)) << endl;
		}
}

void Main::on_callStack_currentItemChanged()
{
	updateDebugger();
}

void Main::alterDebugStateGroup(bool _enable) const
{
	ui->debugStep->setEnabled(_enable);
	ui->debugStepInto->setEnabled(_enable);
	ui->debugStepOut->setEnabled(_enable);
	ui->debugStepBackInto->setEnabled(_enable);
	ui->debugStepBackOut->setEnabled(_enable);
	ui->dumpTrace->setEnabled(_enable);
	ui->dumpTraceStorage->setEnabled(_enable);
	ui->debugStepBack->setEnabled(_enable);
	ui->debugPanel->setEnabled(_enable);
}

void Main::debugFinished()
{
	m_codes.clear();
	m_pcWarp.clear();
	m_history.clear();
	m_lastLevels.clear();
	m_lastCode = h256();
	ui->callStack->clear();
	ui->debugCode->clear();
	ui->debugStack->clear();
	ui->debugMemory->setHtml("");
	ui->debugStorage->setHtml("");
	ui->debugStateInfo->setText("");
	alterDebugStateGroup(false);
//	ui->send->setEnabled(true);
}

void Main::initDebugger()
{
//	ui->send->setEnabled(false);
	if (m_history.size())
	{
		alterDebugStateGroup(true);
		ui->debugCode->setEnabled(false);
		ui->debugTimeline->setMinimum(0);
		ui->debugTimeline->setMaximum(m_history.size());
		ui->debugTimeline->setValue(0);
	}
}

void Main::on_debugTimeline_valueChanged()
{
	updateDebugger();
}

QString Main::prettyU256(eth::u256 _n) const
{
	unsigned inc = 0;
	QString raw;
	ostringstream s;
	if (!(_n >> 64))
		s << "<span style=\"color: #448\">0x</span><span style=\"color: #008\">" << (uint64_t)_n << "</span>";
	else if (!~(_n >> 64))
		s << "<span style=\"color: #448\">0x</span><span style=\"color: #008\">" << (int64_t)_n << "</span>";
	else if (_n >> 200 == 0)
	{
		Address a = right160(_n);
		QString n = pretty(a);
		if (n.isNull())
			s << "<span style=\"color: #844\">0x</span><span style=\"color: #800\">" << a << "</span>";
		else
			s << "<span style=\"font-weight: bold; color: #800\">" << n.toHtmlEscaped().toStdString() << "</span> (<span style=\"color: #844\">0x</span><span style=\"color: #800\">" << a.abridged() << "</span>)";
	}
	else if ((raw = fromRaw((h256)_n, &inc)).size())
		return "<span style=\"color: #484\">\"</span><span style=\"color: #080\">" + raw.toHtmlEscaped() + "</span><span style=\"color: #484\">\"" + (inc ? " + " + QString::number(inc) : "") + "</span>";
	else
		s << "<span style=\"color: #466\">0x</span><span style=\"color: #044\">" << (h256)_n << "</span>";
	return QString::fromStdString(s.str());
}

void Main::updateDebugger()
{
	if (m_history.size())
	{
		WorldState const& nws = m_history[min((int)m_history.size() - 1, ui->debugTimeline->value())];
		WorldState const& ws = ui->callStack->currentRow() > 0 ? *nws.levels[nws.levels.size() - ui->callStack->currentRow()] : nws;

		if (ui->debugTimeline->value() >= m_history.size())
		{
			if (ws.gasCost > ws.gas)
				ui->debugMemory->setHtml("<h3>OUT-OF-GAS</h3>");
			else if (ws.inst == Instruction::RETURN && ws.stack.size() >= 2)
			{
				unsigned from = (unsigned)ws.stack.back();
				unsigned size = (unsigned)ws.stack[ws.stack.size() - 2];
				unsigned o = 0;
				bytes out(size, 0);
				for (; o < size && from + o < ws.memory.size(); ++o)
					out[o] = ws.memory[from + o];
				ui->debugMemory->setHtml("<h3>RETURN</h3>" + QString::fromStdString(eth::memDump(out, 16, true)));
			}
			else if (ws.inst == Instruction::STOP)
				ui->debugMemory->setHtml("<h3>STOP</h3>");
			else if (ws.inst == Instruction::SUICIDE && ws.stack.size() >= 1)
				ui->debugMemory->setHtml("<h3>SUICIDE</h3>0x" + QString::fromStdString(toString(right160(ws.stack.back()))));
			else
				ui->debugMemory->setHtml("<h3>EXCEPTION</h3>");

			ostringstream ss;
			ss << dec << "EXIT  |  GAS: " << dec << max<eth::bigint>(0, (eth::bigint)ws.gas - ws.gasCost);
			ui->debugStateInfo->setText(QString::fromStdString(ss.str()));
			ui->debugStorage->setHtml("");
			ui->debugCallData->setHtml("");
			m_lastData = h256();
			ui->callStack->clear();
			m_lastLevels.clear();
			ui->debugCode->clear();
			m_lastCode = h256();
			ui->debugStack->setHtml("");
		}
		else
		{
			if (m_lastLevels != nws.levels || !ui->callStack->count())
			{
				m_lastLevels = nws.levels;
				ui->callStack->clear();
				for (unsigned i = 0; i <= nws.levels.size(); ++i)
				{
					WorldState const& s = i ? *nws.levels[nws.levels.size() - i] : nws;
					ostringstream out;
					out << s.cur.abridged();
					if (i)
						out << " " << c_instructionInfo.at(s.inst).name << " @0x" << hex << s.curPC;
					ui->callStack->addItem(QString::fromStdString(out.str()));
				}
			}

			if (ws.code != m_lastCode)
			{
				bytes const& code = m_codes[ws.code];
				QListWidget* dc = ui->debugCode;
				dc->clear();
				m_pcWarp.clear();
				for (unsigned i = 0; i <= code.size(); ++i)
				{
					byte b = i < code.size() ? code[i] : 0;
					try
					{
						QString s = c_instructionInfo.at((Instruction)b).name;
						ostringstream out;
						out << hex << setw(4) << setfill('0') << i;
						m_pcWarp[i] = dc->count();
						if (b >= (byte)Instruction::PUSH1 && b <= (byte)Instruction::PUSH32)
						{
							unsigned bc = b - (byte)Instruction::PUSH1 + 1;
							s = "PUSH 0x" + QString::fromStdString(toHex(bytesConstRef(&code[i + 1], bc)));
							i += bc;
						}
						dc->addItem(QString::fromStdString(out.str()) + "  "  + s);
					}
					catch (...)
					{
						break;	// probably hit data segment
					}
				}
				m_lastCode = ws.code;
			}

			if (ws.callData != m_lastData)
			{
				m_lastData = ws.callData;
				if (ws.callData)
				{
					assert(m_codes.count(ws.callData));
					ui->debugCallData->setHtml(QString::fromStdString(eth::memDump(m_codes[ws.callData], 16, true)));
				}
				else
					ui->debugCallData->setHtml("");
			}

			QString stack;
			for (auto i: ws.stack)
				stack.prepend("<div>" + prettyU256(i) + "</div>");
			ui->debugStack->setHtml(stack);
			ui->debugMemory->setHtml(QString::fromStdString(eth::memDump(ws.memory, 16, true)));
			assert(m_codes.count(ws.code));

			if (m_codes[ws.code].size() >= (unsigned)ws.curPC)
			{
				int l = m_pcWarp[(unsigned)ws.curPC];
				ui->debugCode->setCurrentRow(max(0, l - 5));
				ui->debugCode->setCurrentRow(min(ui->debugCode->count() - 1, l + 5));
				ui->debugCode->setCurrentRow(l);
			}
			else
				cwarn << "PC (" << (unsigned)ws.curPC << ") is after code range (" << m_codes[ws.code].size() << ")";

			ostringstream ss;
			ss << dec << "STEP: " << ws.steps << "  |  PC: 0x" << hex << ws.curPC << "  :  " << c_instructionInfo.at(ws.inst).name << "  |  ADDMEM: " << dec << ws.newMemSize << " words  |  COST: " << dec << ws.gasCost <<  "  |  GAS: " << dec << ws.gas;
			ui->debugStateInfo->setText(QString::fromStdString(ss.str()));
			stringstream s;
			for (auto const& i: ws.storage)
				s << "@" << prettyU256(i.first).toStdString() << "&nbsp;&nbsp;&nbsp;&nbsp;" << prettyU256(i.second).toStdString() << "<br/>";
			ui->debugStorage->setHtml(QString::fromStdString(s.str()));
		}
	}
}

// extra bits needed to link on VS
#ifdef _MSC_VER

// include moc file, ofuscated to hide from automoc
#include\
"moc_MainWin.cpp"

#include\
"moc_MiningView.cpp"

#endif
