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
/** @file BlockList.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "BlockList.h"
#include <fstream>
#include <QFileDialog>
#include <QTimer>
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include <libethashseal/EthashAux.h>
#include <libaleth/Debugger.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
#include "ui_BlockList.h"

using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(BlockList);

BlockList::BlockList(ZeroFace* _m):
	Plugin(_m, "Blockchain"),
	m_ui(new Ui::BlockList)
{
	dock(Qt::RightDockWidgetArea, "Blockchain")->setWidget(new QWidget());
	m_ui->setupUi(dock()->widget());
	connect(dock(), SIGNAL(visibilityChanged(bool)), SLOT(refreshBlocks()));
	connect(m_ui->blocks, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(refreshInfo()));
	connect(m_ui->debugCurrent, SIGNAL(pressed()), SLOT(debugCurrent()));
	connect(m_ui->dumpPre, &QPushButton::pressed, [&](){ dumpState(false); });
	connect(m_ui->dumpPost, &QPushButton::pressed, [&](){ dumpState(true); });
	connect(m_ui->filter, SIGNAL(textChanged(QString)), SLOT(filterChanged()));

	m_newBlockWatch = aleth()->installWatch(ChainChangedFilter, [=](LocalisedLogEntries const&){
		refreshBlocks();
	});
	m_pendingWatch = aleth()->installWatch(PendingChangedFilter, [=](LocalisedLogEntries const&){
		refreshBlocks();
	});
	refreshBlocks();
}

BlockList::~BlockList()
{
	aleth()->uninstallWatch(m_newBlockWatch);
	aleth()->uninstallWatch(m_pendingWatch);
}

void BlockList::filterChanged()
{
	static QTimer* s_delayed = nullptr;
	if (!s_delayed)
	{
		s_delayed = new QTimer(this);
		s_delayed->setSingleShot(true);
		connect(s_delayed, SIGNAL(timeout()), SLOT(refreshBlocks()));
	}
	s_delayed->stop();
	s_delayed->start(200);
}

void BlockList::refreshBlocks()
{
	if (!dock()->isVisible())// || !tabifiedDockWidgets(m_ui->blockChainDockWidget).isEmpty()))
		return;

	DEV_TIMED_FUNCTION_ABOVE(500);
	cwatch << "refreshBlockChain()";

	bool found = false;
	m_inhibitInfoRefresh = true;

	// TODO: keep the same thing highlighted.
	// TODO: refactor into MVC
	// TODO: use get by hash/number
	// TODO: transactions

	auto const& bc = ethereum()->blockChain();
	QStringList filters = m_ui->filter->text().toLower().split(QRegExp("\\s+"), QString::SkipEmptyParts);

	h256Hash blocks;
	for (QString f: filters)
	{
		if (f == "pending")
			blocks.insert(PendingBlockHash);
		if (f.size() == 64)
		{
			h256 h(f.toStdString());
			if (bc.isKnown(h))
				blocks.insert(h);
			for (auto const& b: bc.withBlockBloom(LogBloom().shiftBloom<3>(sha3(h)), 0, -1))
				blocks.insert(bc.numberHash(b));
		}
		else if (f.toLongLong() <= bc.number())
			blocks.insert(bc.numberHash((unsigned)f.toLongLong()));
		else if (f.size() == 40)
		{
			Address h(f.toStdString());
			for (auto const& b: bc.withBlockBloom(LogBloom().shiftBloom<3>(sha3(h)), 0, -1))
				blocks.insert(bc.numberHash(b));
		}
	}

	h256 selectedHash = m_ui->blocks->count() ? h256((byte const*)m_ui->blocks->currentItem()->data(Qt::UserRole).toByteArray().data(), h256::ConstructFromPointer) : h256();
	unsigned selectedIndex = (m_ui->blocks->count() && !m_ui->blocks->currentItem()->data(Qt::UserRole + 1).isNull()) ? (unsigned)m_ui->blocks->currentItem()->data(Qt::UserRole + 1).toInt() : (unsigned)-1;

	m_ui->blocks->clear();

	auto showBlock = [&](h256 const& h) {
		QListWidgetItem* blockItem;
		if (h == PendingBlockHash)
			blockItem = new QListWidgetItem(QString("#%1 <pending>").arg(ethereum()->pendingDetails().number), m_ui->blocks);
		else
		{
			auto d = bc.details(h);
			blockItem = new QListWidgetItem(QString("#%1 %2").arg(d.number).arg(h.abridged().c_str()), m_ui->blocks);
		}
		if (selectedHash == h && selectedIndex == (unsigned)-1)
		{
			m_ui->blocks->setCurrentItem(blockItem);
			found = true;
		}
		auto hba = QByteArray((char const*)h.data(), h.size);
		blockItem->setData(Qt::UserRole, hba);

		unsigned n = 0;
		try {
			for (Transaction const& t: (h == PendingBlockHash) ? ethereum()->pending() : ethereum()->transactions(h))
			{
				QString s = t.receiveAddress() ?
					QString("    %2 %5> %3: %1 [%4]")
						.arg(formatBalance(t.value()).c_str())
						.arg(QString::fromStdString(aleth()->toReadable(t.safeSender())))
						.arg(QString::fromStdString(aleth()->toReadable(t.receiveAddress())))
						.arg((unsigned)t.nonce())
						.arg(ethereum()->codeAt(t.receiveAddress()).size() ? '*' : '-') :
					QString("    %2 +> %3: %1 [%4]")
						.arg(formatBalance(t.value()).c_str())
						.arg(QString::fromStdString(aleth()->toReadable(t.safeSender())))
						.arg(QString::fromStdString(aleth()->toReadable(right160(sha3(rlpList(t.safeSender(), t.nonce()))))))
						.arg((unsigned)t.nonce());
				QListWidgetItem* txItem = new QListWidgetItem(s, m_ui->blocks);
				txItem->setData(Qt::UserRole, hba);
				txItem->setData(Qt::UserRole + 1, n);
				if (selectedHash == h && selectedIndex == n)
				{
					m_ui->blocks->setCurrentItem(txItem);
					found = true;
				}
				n++;
			}
		}
		catch (...) {}
	};

	if (filters.empty())
	{
		unsigned i = 10;
		showBlock(PendingBlockHash);
		for (auto h = bc.currentHash(); bc.details(h) && i; h = bc.details(h).parent, --i)
		{
			showBlock(h);
			if (h == bc.genesisHash())
				break;
		}
	}
	else
		for (auto const& h: blocks)
			showBlock(h);

	m_inhibitInfoRefresh = false;

	if (!m_ui->blocks->currentItem() || !found)
	{
		m_ui->blocks->setCurrentRow(0);
		refreshInfo();
	}
}

void BlockList::refreshInfo()
{
	if (m_inhibitInfoRefresh)
		return;
	m_ui->info->clear();
	m_ui->debugCurrent->setEnabled(false);
	if (auto item = m_ui->blocks->currentItem())
	{
		stringstream s;
		h256 h;
		auto hba = item->data(Qt::UserRole).toByteArray();
		if (hba.size() == 32)
			h = h256((byte const*)hba.data(), h256::ConstructFromPointer);


		BlockDetails details;
		bytes blockData;
		RLP block;
		BlockHeader header;
		if (h == PendingBlockHash)
		{
			header = ethereum()->pendingInfo();
			details = ethereum()->pendingDetails();
		}
		else
		{
			details = ethereum()->blockChain().details(h);
			blockData = ethereum()->blockChain().block(h);
			block = RLP(blockData);
			header = BlockHeader(blockData);
		}

		if (item->data(Qt::UserRole + 1).isNull())
		{
			char timestamp[64];
			time_t rawTime = (time_t)(uint64_t)header.timestamp();
			strftime(timestamp, 64, "%c", localtime(&rawTime));
			if (h == PendingBlockHash)
				s << "<h3>Pending</h3>";
			else
				s << "<h3>" << h << "</h3>";
			s << "<h4>#" << header.number();
			s << "&nbsp;&emsp;&nbsp;<b>" << timestamp << "</b></h4>";
			try
			{
				RLP r(header.extraData());
				if (r[0].toInt<int>() == 0)
					s << "<div>Sealing client: <b>" << htmlEscaped(r[1].toString()) << "</b>" << "</div>";
			}
			catch (...) {}
			s << "<div>D/TD: <b>" << header.difficulty() << "</b>/<b>" << details.totalDifficulty << "</b> = 2^" << log2((double)header.difficulty()) << "/2^" << log2((double)details.totalDifficulty) << "</div>";
			s << "&nbsp;&emsp;&nbsp;Children: <b>" << details.children.size() << "</b></div>";
			s << "<div>Gas used/limit: <b>" << header.gasUsed() << "</b>/<b>" << header.gasLimit() << "</b>" << "</div>";
			s << "<div>Beneficiary: <b>" << htmlEscaped(aleth()->toReadable(header.author())) << " " << header.author() << "</b>" << "</div>";
			s << "<div>Difficulty: <b>" << header.difficulty() << "</b>" << "</div>";
			if (h != PendingBlockHash)
			{
				for (auto const& i: ethereum()->sealEngine()->jsInfo(header))
					s << "<div>" << i.first << ": <b>" << htmlEscaped(i.second) << "</b>" << "</div>";
				s << "<div>Hash w/o nonce: <b>" << header.hash(WithoutSeal) << "</b>" << "</div>";
				if (header.number())
					s << "<div>Parent: <b>" << header.parentHash() << "</b>" << "</div>";
				else
				{
					s << "<div>Proof-of-Work: <b><i>Phil has nothing to prove</i></b></div>";
					s << "<div>Parent: <b><i>It was a virgin birth</i></b></div>";
				}
			}
			s << "<div>State root: " << ETH_HTML_SPAN(ETH_HTML_MONO) << header.stateRoot().hex() << "</span></div>";
			s << "<div>Extra data: " << ETH_HTML_SPAN(ETH_HTML_MONO) << toHex(header.extraData()) << "</span></div>";
			if (!!header.logBloom())
				s << "<div>Log Bloom: " << header.logBloom() << "</div>";
			else
				s << "<div>Log Bloom: <b><i>Uneventful</i></b></div>";
			s << "<div>Transactions: <b>" << block[1].itemCount() << "</b> @<b>" << header.transactionsRoot() << "</b>" << "</div>";
			s << "<div>Uncles: <b>" << block[2].itemCount() << "</b> @<b>" << header.sha3Uncles() << "</b>" << "</div>";
			if (h != PendingBlockHash)
				for (auto u: block[2])
				{
					BlockHeader uncle(u.data(), HeaderData);
					char const* line = "<div><span style=\"margin-left: 2em\">&nbsp;</span>";
					s << line << "Hash: <b>" << uncle.hash() << "</b>" << "</div>";
					s << line << "Parent: <b>" << uncle.parentHash() << "</b>" << "</div>";
					s << line << "Number: <b>" << uncle.number() << "</b>" << "</div>";
					s << line << "Author: <b>" << htmlEscaped(aleth()->toReadable(uncle.author())) << " " << uncle.author() << "</b>" << "</div>";
					for (auto const& i: ethereum()->sealEngine()->jsInfo(header))
						s << line << i.first << ": <b>" << htmlEscaped(i.second) << "</b>" << "</div>";
					s << line << "Hash w/o nonce: <b>" << uncle.hash(WithoutSeal) << "</b>" << "</div>";
					s << line << "Difficulty: <b>" << uncle.difficulty() << "</b>" << "</div>";
				}
			if (header.parentHash())
				s << "<div>Pre: <b>" << BlockHeader(ethereum()->blockChain().block(header.parentHash())).stateRoot() << "</b>" << "</div>";
			else
				s << "<div>Pre: <b><i>Nothing is before Phil</i></b>" << "</div>";

			s << "<div>Receipts: @<b>" << header.receiptsRoot() << "</b>:" << "</div>";
			unsigned ii = 0;
			for (auto const& i: block[1])
			{
				TransactionReceipt r = h == PendingBlockHash ? ethereum()->postState().receipt(ii) : ethereum()->blockChain().receipts(h).receipts[ii];
				s << "<div>" << sha3(i.data()).abridged() << ": <b>" << r.stateRoot() << "</b> [<b>" << r.gasUsed() << "</b> used]" << "</div>";
				++ii;
			}
			s << "<div>Post: <b>" << header.stateRoot() << "</b>" << "</div>";
			if (h != PendingBlockHash)
			{
				s << "<div>Dump: " ETH_HTML_SPAN(ETH_HTML_MONO) << toHex(block[0].data()) << "</span>" << "</div>";
				s << "<div>Receipts-Hex: " ETH_HTML_SPAN(ETH_HTML_MONO) << toHex(ethereum()->blockChain().receipts(h).rlp()) << "</span></div>";
			}
		}
		else
		{
			unsigned txi = item->data(Qt::UserRole + 1).toInt();
			Transaction tx = h == PendingBlockHash ? ethereum()->pending()[txi] : Transaction(block[1][txi].data(), CheckTransaction::Everything);
			auto ss = tx.safeSender();
			h256 th = sha3(rlpList(ss, tx.nonce()));
			TransactionReceipt receipt = h == PendingBlockHash ? ethereum()->postState().receipt(txi) : ethereum()->blockChain().receipts(h).receipts[txi];

			s << "<h3>" << th << "</h3>";
			s << "<h4>" << h << "[<b>" << txi << "</b>]</h4>";
			s << "<div>From: <b>" << htmlEscaped(aleth()->toReadable(ss)) << " " << ss << "</b>" << "</div>";
			if (tx.isCreation())
				s << "<div>Creates: <b>" << htmlEscaped(aleth()->toReadable(right160(th))) << "</b> " << right160(th) << "</div>";
			else
				s << "<div>To: <b>" << htmlEscaped(aleth()->toReadable(tx.receiveAddress())) << "</b> " << tx.receiveAddress() << "</div>";
			s << "<div>Value: <b>" << formatBalance(tx.value()) << "</b>" << "</div>";
			s << "&nbsp;&emsp;&nbsp;#<b>" << tx.nonce() << "</b>" << "</div>";
			s << "<div>Gas price: <b>" << formatBalance(tx.gasPrice()) << "</b>" << "</div>";
			s << "<div>Gas: <b>" << tx.gas() << "</b>" << "</div>";
			s << "<div>V: <b>" << hex << nouppercase << (int)tx.signature().v << " + 27</b>" << "</div>";
			s << "<div>R: <b>" << hex << nouppercase << tx.signature().r << "</b>" << "</div>";
			s << "<div>S: <b>" << hex << nouppercase << tx.signature().s << "</b>" << "</div>";
			s << "<div>Msg: <b>" << tx.sha3(eth::WithoutSignature) << "</b>" << "</div>";
			if (!tx.data().empty())
			{
				if (tx.isCreation())
					s << "<h4>Code</h4>" << disassemble(tx.data());
				else
					s << "<h4>Data</h4>" << dev::memDump(tx.data(), 16, true);
			}
			s << "<div>Hex: " ETH_HTML_SPAN(ETH_HTML_MONO) << toHex(block[1][txi].data()) << "</span></div>";
			s << "<hr/>";
			if (!!receipt.bloom())
				s << "<div>Log Bloom: " << receipt.bloom() << "</div>";
			else
				s << "<div>Log Bloom: <b><i>Uneventful</i></b></div>";
			s << "<div>Gas Used: <b>" << receipt.gasUsed() << "</b></div>";
			s << "<div>End State: <b>" << receipt.stateRoot().abridged() << "</b></div>";
			auto r = receipt.rlp();
			s << "<div>Receipt: " << toString(RLP(r)) << "</div>";
			s << "<div>Receipt-Hex: " ETH_HTML_SPAN(ETH_HTML_MONO) << toHex(receipt.rlp()) << "</span></div>";
			s << "<h4>Diff</h4>" << aleth()->toHTML(h == PendingBlockHash ? ethereum()->diff(txi, PendingBlock) : ethereum()->diff(txi, h));

			m_ui->debugCurrent->setEnabled(true);
		}
		m_ui->info->appendHtml(QString::fromStdString(s.str()));
		m_ui->info->moveCursor(QTextCursor::Start);
	}
}

void BlockList::debugCurrent()
{
	// TODO: abstract this top bit.
	if (QListWidgetItem* item = m_ui->blocks->currentItem())
		if (!item->data(Qt::UserRole + 1).isNull())
		{
			auto hba = item->data(Qt::UserRole).toByteArray();
			assert(hba.size() == 32);
			h256 h = h256((byte const*)hba.data(), h256::ConstructFromPointer);
			unsigned txi = item->data(Qt::UserRole + 1).toInt();

			bytes t = h == PendingBlockHash ? ethereum()->pending()[txi].rlp() : ethereum()->blockChain().transaction(h, txi);
			State s = h == PendingBlockHash ? ethereum()->state(txi) : ethereum()->state(txi, h);
			BlockHeader bi = h == PendingBlockHash ? ethereum()->pendingInfo() : ethereum()->blockChain().info(h);
			Executive e(s, EnvInfo(bi, ethereum()->blockChain().lastHashes(bi.parentHash())), aleth()->ethereum()->sealEngine());
			Debugger dw(aleth(), zero());
			dw.populate(e, Transaction(t, CheckTransaction::Everything));
			dw.exec();
		}
}

void BlockList::dumpState(bool _post)
{
	// TODO: abstract this top bit.
#if ETH_FATDB
	if (QListWidgetItem* item = m_ui->blocks->currentItem())
		if (!item->data(Qt::UserRole + 1).isNull())
		{
			h256 h;
			if (!item->data(Qt::UserRole).isNull())
			{
				auto hba = item->data(Qt::UserRole).toByteArray();
				assert(hba.size() == 32);
				h = h256((byte const*)hba.data(), h256::ConstructFromPointer);
			}
			unsigned txi = item->data(Qt::UserRole + 1).toInt() + (_post ? 1 : 0);

			QString fn = QFileDialog::getSaveFileName(zero(), "Select file to output state dump");
			ofstream f(fn.toStdString());
			if (f.is_open())
			{
				if (item->data(Qt::UserRole + 1).isNull())
					(h == PendingBlockHash ? ethereum()->postState() : ethereum()->block(h)).state().streamJSON(f);
				else
					(h == PendingBlockHash ? ethereum()->state(txi) : ethereum()->state(txi, h)).streamJSON(f);
			}
		}
#else
	(void)_post;
#endif // ETH_FATDB
}

