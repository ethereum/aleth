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
/** @file Whisper.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Whisper.h"
#include <QSettings>
#include <QShortcut>
#include <QMessageBox>
#include <libethereum/Client.h>
#include <libethereum/Utility.h>
#include <libwhisper/WhisperHost.h>
#include <libweb3jsonrpc/Whisper.h>
#include <libwebthree/WebThree.h>
#include <libaleth/AlethWhisper.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
#include "WhisperPeers.h"
#include "ui_Whisper.h"

using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(Whisper);

static std::string const c_chatPluginName("WhisperPeers");

static Public stringToPublic(QString const& _a)
{
	string sn = _a.toStdString();
	if (_a.size() == sizeof(Public) * 2)
		return Public(fromHex(_a.toStdString()));
	else if (_a.size() == sizeof(Public) * 2 + 2 && _a.startsWith("0x"))
		return Public(fromHex(_a.mid(2).toStdString()));
	else
		return Public();
}

static shh::Topics stringToTopics(QString _s)
{
	return shh::BuildTopic(_s.toStdString());

	//shh::BuildTopic ret;
	//QStringList tx = _s.split("|", QString::SkipEmptyParts);
	//for (auto t: tx)
	//	ret.shift(t.toStdString());
	//return ret.toTopics();
}

static shh::Topics topicFromText(QString _s)
{
	shh::BuildTopic ret;
	while (_s.size())
	{
		QRegExp r("(@|\\$)?\"([^\"]*)\"(\\s.*)?");
		QRegExp d("(@|\\$)?([0-9]+)(\\s*(ether)|(finney)|(szabo))?(\\s.*)?");
		QRegExp h("(@|\\$)?(0x)?(([a-fA-F0-9])+)(\\s.*)?");
		bytes part;
		if (r.exactMatch(_s))
		{
			for (auto i: r.cap(2))
				part.push_back((byte)i.toLatin1());
			if (r.cap(1) != "$")
				for (int i = r.cap(2).size(); i < 32; ++i)
					part.push_back(0);
			else
				part.push_back(0);
			_s = r.cap(3);
		}
		else if (d.exactMatch(_s))
		{
			u256 v(d.cap(2).toStdString());
			if (d.cap(6) == "szabo")
				v *= szabo;
			else if (d.cap(5) == "finney")
				v *= finney;
			else if (d.cap(4) == "ether")
				v *= ether;
			bytes bs = dev::toCompactBigEndian(v);
			if (d.cap(1) != "$")
				for (auto i = bs.size(); i < 32; ++i)
					part.push_back(0);
			for (auto b: bs)
				part.push_back(b);
			_s = d.cap(7);
		}
		else if (h.exactMatch(_s))
		{
			bytes bs = fromHex((((h.cap(3).size() & 1) ? "0" : "") + h.cap(3)).toStdString());
			if (h.cap(1) != "$")
				for (auto i = bs.size(); i < 32; ++i)
					part.push_back(0);
			for (auto b: bs)
				part.push_back(b);
			_s = h.cap(5);
		}
		else
			_s = _s.mid(1);
		ret.shift(part);
	}
	return ret;
}

Whisper::Whisper(ZeroFace* _m):
	Plugin(_m, "Whisper"),
	m_ui(new Ui::Whisper)
{
	dock(Qt::RightDockWidgetArea, "Whisper")->setWidget(new QWidget);
	m_ui->setupUi(dock()->widget());
	connect(addMenuItem("New Whisper Identity", "menuAccounts", true), &QAction::triggered, this, &Whisper::onNewIdentityTriggered);
	connect(zero()->whisperFace(), &AlethWhisper::onNewId, this, &Whisper::addNewId);
	connect(m_ui->post, SIGNAL(clicked()), this, SLOT(onPostClicked()));
	connect(m_ui->forgetTopics, SIGNAL(clicked()), this, SLOT(onForgetTopicsClicked()));
	connect(m_ui->forgetDestinations, SIGNAL(clicked()), this, SLOT(onForgetDestinationsClicked()));

	QShortcut* shortcutSend1 = new QShortcut(QKeySequence("Ctrl+Enter"), m_ui->shhData);
	QShortcut* shortcutSend2 = new QShortcut(QKeySequence("Ctrl+Return"), m_ui->shhData);
	QObject::connect(shortcutSend1, SIGNAL(activated()), this, SLOT(onPostClicked()));
	QObject::connect(shortcutSend2, SIGNAL(activated()), this, SLOT(onPostClicked()));
}

void Whisper::readSettings(QSettings const& _s)
{
	m_myIdentities.clear();
	QByteArray b = _s.value("identities").toByteArray();
	if (!b.isEmpty())
	{
		Secret k;
		for (unsigned i = 0; i < b.size() / sizeof(Secret); ++i)
		{
			memcpy(k.writable().data(), b.data() + i * sizeof(Secret), sizeof(Secret));
			if (!count(m_myIdentities.begin(), m_myIdentities.end(), KeyPair(k)))
				m_myIdentities.append(KeyPair(k));
		}
	}

	zero()->whisperFace()->setIdentities(keysAsVector(m_myIdentities));

	int ttl = _s.value("ttl").toInt();
	if (ttl > 0)
		m_ui->shhTtl->setValue(ttl);

	int PoW = _s.value("PoW").toInt();
	if (PoW > 0)
		m_ui->shhWork->setValue(PoW);

	refreshWhisper();
}

void Whisper::writeSettings(QSettings& _s)
{
	QByteArray b;
	b.resize(sizeof(Secret) * m_myIdentities.size());
	auto p = b.data();
	for (auto i: m_myIdentities)
	{
		memcpy(p, &(i.secret()), sizeof(Secret));
		p += sizeof(Secret);
	}

	_s.setValue("identities", b);
	_s.setValue("ttl", m_ui->shhTtl->value());
	_s.setValue("PoW", m_ui->shhWork->value());
}

void Whisper::addNewId(QString _ids)
{
	KeyPair kp(jsToSecret(_ids.toStdString()));
	m_myIdentities.push_back(kp);
	zero()->whisperFace()->setIdentities(keysAsVector(m_myIdentities));
	refreshWhisper();
}

void Whisper::refreshWhisper()
{
	m_ui->shhFrom->clear();
	for (auto i: zero()->whisperFace()->ids())
		m_ui->shhFrom->addItem(QString::fromStdString(toHex(i.first.ref())));

	m_ui->shhFrom->addItem(QString());
}

void Whisper::onNewIdentityTriggered()
{
	KeyPair kp = KeyPair::create();
	m_myIdentities.append(kp);
	zero()->whisperFace()->setIdentities(keysAsVector(m_myIdentities));
	refreshWhisper();
}

void Whisper::onPostClicked()
{
	QString const strTopic = m_ui->shhTopic->currentText();
	QString const text = m_ui->shhData->toPlainText();
	QString const strDest = m_ui->shhTo->currentText();
	Public dest = stringToPublic(strDest);

	noteTopic(strTopic);
	m_ui->shhData->clear();
	m_ui->shhData->setFocus();

	if (dest)
	{
		if (m_ui->shhTo->findText(strDest) < 0)
			m_ui->shhTo->addItem(strDest);
	}
	else if (!strDest.isEmpty())
	{
		// don't allow unencrypted messages, unless it was explicitly intended
		QMessageBox box(QMessageBox::Warning, "Warning", "Invalid destination address!");
		box.setInformativeText("Destination address must be either valid or blank.");
		box.exec();
		return;
	}

	if (text.isEmpty())
		return;

	int const msgSizeLimit = 2000;
	if (text.size() > msgSizeLimit)
	{
		QMessageBox box(QMessageBox::Warning, "Warning", "Message too large!");
		box.setInformativeText(QString("Single message should not exceed %1 characters.").arg(msgSizeLimit));
		box.exec();
		return;
	}

	shh::Message m;
	m.setTo(dest);
	m.setPayload(asBytes(text.toStdString()));

	Secret from;
	Public f = stringToPublic(m_ui->shhFrom->currentText());
	if (zero()->whisperFace()->ids().count(f))
		from = zero()->whisperFace()->ids().at(f);

	shh::Topics topic = stringToTopics(strTopic);
	web3()->whisper()->inject(m.seal(from, topic, m_ui->shhTtl->value(), m_ui->shhWork->value()));
}

void Whisper::onForgetDestinationsClicked()
{
	m_ui->shhTo->clear();
}

void Whisper::onForgetTopicsClicked()
{
	m_ui->shhTopic->clear();
}

void Whisper::noteTopic(QString const& _topic)
{
	if (_topic.isEmpty())
		return;

	if (m_ui->shhTopic->findText(_topic) < 0)
		m_ui->shhTopic->addItem(_topic);

	auto x = zero()->findPlugin(c_chatPluginName);
	WhisperPeers* wp = dynamic_cast<WhisperPeers*>(x.get());
	if (wp)
		wp->noteTopic(_topic);

	//QStringList tx = _topic.split("|", QString::SkipEmptyParts);
	//if (tx.size() > 1 && m_ui->shhTopic->findText(_topic) < 0)
	//	m_ui->shhTopic->addItem(_topic);
	//for (auto t: tx)
	//{
	//	if (m_ui->shhTopic->findText(t) < 0)
	//		m_ui->shhTopic->addItem(t);
	//	if (wp)
	//		wp->noteTopic(t);
	//}
}
