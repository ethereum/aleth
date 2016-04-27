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
/** @file AccountHolder.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "AccountHolder.h"
#include <QMessageBox>
#include <QAbstractButton>
#include <libdevcore/Log.h>
#include <libethcore/KeyManager.h>
#include <libwebthree/WebThree.h>
#include "AlethFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

aleth::AccountHolder::AccountHolder(AlethFace* _aleth):
	eth::AccountHolder([=](){ return _aleth->ethereum(); }),
	m_aleth(_aleth)
{
	startTimer(500);
}

bool aleth::AccountHolder::showAuthenticationPopup(string const& _title, string const& _text)
{
	QMessageBox userInput;
	userInput.setText(QString::fromStdString(_title));
	userInput.setInformativeText(QString::fromStdString(_text));
	userInput.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	userInput.button(QMessageBox::Ok)->setText("Allow");
	userInput.button(QMessageBox::Cancel)->setText("Reject");
	userInput.setDefaultButton(QMessageBox::Cancel);
	return userInput.exec() == QMessageBox::Ok;
}

TransactionNotification aleth::AccountHolder::authenticate(TransactionSkeleton const& _t)
{
	UniqueGuard l(x_queued);
	auto id = m_nextQueueID++;
	m_queued.push(make_pair(_t, id));
	while (!m_queueOutput.count(id))
		m_queueCondition.wait(l);
	TransactionNotification ret = m_queueOutput[id];
	m_queueOutput.erase(id);
	return ret;
}

void aleth::AccountHolder::doValidations()
{
	Guard l(x_queued);
	while (!m_queued.empty())
	{
		TransactionSkeleton t;
		unsigned id;
		tie(t, id) = m_queued.front();
		m_queued.pop();

		TransactionNotification n;
		bool proxy = isProxyAccount(t.from);
		if (!proxy && !isRealAccount(t.from))
		{
			cwarn << "Trying to send from non-existant account" << t.from;
			n.r = TransactionRepercussion::UnknownAccount;
		}
		else
		{
			// TODO: determine gas price.
			if (validateTransaction(t, proxy))
			{
				if (proxy)
				{
					queueTransaction(t);
					n.r = TransactionRepercussion::ProxySuccess;
				}
				else
				{
					// sign and submit.
					if (Secret s = m_aleth->retrieveSecret(t.from))
					{
						tie(n.hash, n.created) = m_aleth->ethereum()->submitTransaction(t, s);
						n.r = TransactionRepercussion::Success;
					}
					else
						n.r = TransactionRepercussion::Locked;
				}
			}
			else
			    n.r = TransactionRepercussion::Refused;
		}

		m_queueOutput[id] = n;
	}
	m_queueCondition.notify_all();
}

AddressHash aleth::AccountHolder::realAccounts() const
{
	return m_aleth->keyManager().accountsHash();
}

bool aleth::AccountHolder::validateTransaction(TransactionSkeleton const& _t, bool _toProxy)
{
	if (!m_isEnabled)
		return true;

	return showAuthenticationPopup("Transaction", _t.userReadable(_toProxy,
		[&](TransactionSkeleton const& _t) -> pair<bool, string>
		{
			h256 contractCodeHash = m_aleth->ethereum()->postState().codeHash(_t.to);
			if (contractCodeHash == EmptySHA3)
				return make_pair(false, std::string());
			string userNotice = m_aleth->natSpec().userNotice(contractCodeHash, _t.data);
			return std::make_pair(true, userNotice);
		},
		[&](Address const& _a) -> string { return m_aleth->toReadable(_a); }
	));
}


void aleth::AccountHolder::timerEvent(QTimerEvent*)
{
	doValidations();
}
