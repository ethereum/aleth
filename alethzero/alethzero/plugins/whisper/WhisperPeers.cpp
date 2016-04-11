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
/** @file WhisperPeers.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "WhisperPeers.h"
#include <QSettings>
#include <QScrollBar>
#include <QMutexLocker>
#include <libethereum/Client.h>
#include <libwhisper/WhisperHost.h>
#include <libwebthree/WebThree.h>
#include <libaleth/AlethWhisper.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
#include "ui_WhisperPeers.h"

using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(WhisperPeers);

static QString const c_filterAll("-= all messages =-");

static QString messageToString(shh::Envelope const& _e, shh::Message const& _m, QString const& _topic)
{
	time_t birth = _e.expiry() - _e.ttl();
	QString t(ctime(&birth));
	t.chop(6);
	t = t.right(t.size() - 4);

	QString seal = QString("{%1 -> %2}").arg(_m.from() ? _m.from().abridged().c_str() : "?").arg(_m.to() ? _m.to().abridged().c_str() : "X");
	QString item = QString("[%1, ttl: %2] *%3 [%4] %5").arg(t).arg(_e.ttl()).arg(_e.workProved()).arg(_topic).arg(seal);

	bytes raw = _m.payload();
	if (raw.size())
	{
		QString plaintext = QString::fromUtf8((const char*)(raw.data()), raw.size());
		item += ": ";
		item += plaintext;
	}

	return item;
}

WhisperPeers::WhisperPeers(ZeroFace* _m):
	Plugin(_m, "WhisperPeers"),
	m_ui(new Ui::WhisperPeers),
	m_stopped(false)
{
	dock(Qt::RightDockWidgetArea, "Active Whispers")->setWidget(new QWidget);
	m_ui->setupUi(dock()->widget());
	connect(m_ui->topics, SIGNAL(currentIndexChanged(QString)), this, SLOT(onFilterChanged()));
	connect(m_ui->forgetCurrent, SIGNAL(clicked()), this, SLOT(onForgetCurrentTopicClicked()));
	connect(m_ui->forgetAll, SIGNAL(clicked()), this, SLOT(onForgetAllClicked()));
	connect(m_ui->stop, SIGNAL(clicked()), this, SLOT(onStopClicked()));
	connect(m_ui->clear, SIGNAL(clicked()), this, SLOT(onClearClicked()));
	setDefaultTopics();
	startTimer(1000);
}

WhisperPeers::~WhisperPeers()
{
	onForgetAllClicked();
}

void WhisperPeers::setDefaultTopics()
{
	m_ui->topics->addItem(c_filterAll);
}

bool WhisperPeers::isCurrentTopicAll() const
{
	QString const topic = m_ui->topics->currentText();
	return !topic.compare(c_filterAll);
}

void WhisperPeers::noteTopic(QString const& _topic)
{
	if (m_ui->topics->findText(_topic) < 0)
	{
		m_ui->topics->addItem(_topic);

		if (m_topics.find(_topic) == m_topics.end())
		{
			shh::BuildTopic bt(_topic.toStdString());
			unsigned f = web3()->whisper()->installWatch(bt);
			m_topics[_topic] = f;
		}
	}
}

void WhisperPeers::onForgetAllClicked()
{
	m_ui->topics->clear();
	setDefaultTopics();

	QMutexLocker guard(&m_chatLock);

	for (auto const& i: m_topics)
		web3()->whisper()->uninstallWatch(i.second);

	m_ui->whispers->clear();
	m_topics.clear();
	m_chats.clear();
	m_all.clear();
}

void WhisperPeers::onForgetCurrentTopicClicked()
{
	if (!isCurrentTopicAll())
	{
		QString const& topic = m_ui->topics->currentText();

		auto i = m_topics.find(topic);
		if (i != m_topics.end())
			web3()->whisper()->uninstallWatch(i->second);

		QMutexLocker guard(&m_chatLock);
		m_ui->topics->removeItem(m_ui->topics->currentIndex());
		m_topics.erase(topic);
		m_chats.erase(topic);
	}
}

void WhisperPeers::timerEvent(QTimerEvent*)
{
	if (m_stopped)
		return;

	if (m_chatLock.tryLock())
	{
		refreshWhispers(true);
		m_chatLock.unlock();
	}
}

void WhisperPeers::onFilterChanged()
{
	if (!m_ui->topics->currentText().isEmpty())
	{
		QMutexLocker guard(&m_chatLock);
		m_ui->whispers->clear();
		refreshWhispers(false);
		m_ui->forgetCurrent->setEnabled(!isCurrentTopicAll());
	}

	QCoreApplication::processEvents();
}

void WhisperPeers::refreshWhispers(bool _timerEvent)
{
	if (_timerEvent)
		QCoreApplication::processEvents();

	QString const topic = m_ui->topics->currentText();
	if (!topic.compare(c_filterAll))
		refreshAll(_timerEvent);
	else
		refresh(topic, _timerEvent);
}

void WhisperPeers::onStopClicked()
{
	m_stopped = !m_stopped;
	m_ui->stop->setText(m_stopped ? "Resume" : "Freeze");
}

void WhisperPeers::onClearClicked()
{
	QMutexLocker guard(&m_chatLock);

	QString const topic = m_ui->topics->currentText();
	if (!topic.compare(c_filterAll))
		m_all.clear();
	else
		m_chats[topic].clear();

	m_ui->whispers->clear();
}

void WhisperPeers::addToView(std::multimap<time_t, QString> const& _messages)
{
	auto chat = m_ui->whispers;
	if (!chat)
		return;

	QScrollBar* scroll = chat->verticalScrollBar();
	if (!scroll)
		return;

	bool wasAtBottom = (scroll->maximum() == scroll->sliderPosition());

	// limit the number of messges, may be useful in the future
	//int target = c_maxMessages - static_cast<int>(_messages.size());
	//if (target <= 0)
	//	m_ui->whispers->clear();
	//else
	//	while (m_ui->whispers->count() > target)
	//		delete m_ui->whispers->takeItem(0);

	for (auto const& i: _messages)
	{
		QListWidgetItem* item = new QListWidgetItem;
		item->setText(i.second);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		chat->addItem(item);
	}

	if (wasAtBottom)
		chat->scrollToBottom();
}

void WhisperPeers::refresh(QString const& _topic, bool _timerEvent)
{
	shh::BuildTopic bt(_topic.toStdString());
	unsigned const w = m_topics[_topic];
	multimap<time_t, QString> newMessages;
	multimap<time_t, QString>& chat = m_chats[_topic];

	if (m_topics.find(_topic) == m_topics.end())
	{
		unsigned f = web3()->whisper()->installWatch(bt);
		m_topics[_topic] = f;
		return;
	}

	for (auto i: web3()->whisper()->checkWatch(w))
	{
		shh::Envelope const& e = web3()->whisper()->envelope(i);
		shh::Message msg = e.open(web3()->whisper()->fullTopics(w));

		if (!msg)
			for (pair<Public, Secret> const& i: zero()->whisperFace()->ids())
				if (!!(msg = e.open(bt, i.second)))
					break;

		if (msg)
		{
			time_t birth = e.expiry() - e.ttl();
			QString s = messageToString(e, msg, _topic);
			chat.emplace(birth, s);
			m_all.emplace(birth, s);
			if (_timerEvent)
				newMessages.emplace(birth, s);
		}
	}

	resizeMap(chat);
	resizeMap(m_all);

	if (_timerEvent)
	{
		resizeMap(newMessages);
		addToView(newMessages);
	}
	else
		addToView(chat);
}

void WhisperPeers::refreshAll(bool _timerEvent)
{
	multimap<time_t, QString> newMessages;

	for (auto t = m_topics.begin(); t != m_topics.end(); ++t)
	{
		multimap<time_t, QString>& chat = m_chats[t->first];

		for (auto i: web3()->whisper()->checkWatch(t->second))
		{
			shh::Envelope const& e = web3()->whisper()->envelope(i);
			shh::Message msg = e.open(web3()->whisper()->fullTopics(t->second));

			if (!msg)
				for (pair<Public, Secret> const& i: zero()->whisperFace()->ids())
					if (!!(msg = e.open(shh::Topics(), i.second)))
						break;

			if (msg)
			{
				time_t birth = e.expiry() - e.ttl();
				QString s = messageToString(e, msg, t->first);
				chat.emplace(birth, s);
				m_all.emplace(birth, s);
				if (_timerEvent)
					newMessages.emplace(birth, s);
			}
		}

		resizeMap(chat);
	}

	resizeMap(m_all);

	if (_timerEvent)
	{
		resizeMap(newMessages);
		addToView(newMessages);
	}
	else
		addToView(m_all);
}

void WhisperPeers::resizeMap(std::multimap<time_t, QString>& _map)
{
	while (_map.size() > 5000)
		_map.erase(_map.begin());
}
