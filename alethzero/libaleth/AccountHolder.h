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

#pragma once

#include <queue>
#include <unordered_map>
#include <QObject>
#include <libdevcore/Guards.h>
#include <libweb3jsonrpc/AccountHolder.h>

namespace dev
{
namespace aleth
{

class AlethFace;

class AccountHolder: public QObject, public eth::AccountHolder
{
	Q_OBJECT

public:
	AccountHolder(AlethFace* _aleth);

	void setEnabled(bool _e) { m_isEnabled = _e; }
	bool isEnabled() const { return m_isEnabled; }

private slots:
	void doValidations();

protected:
	// easiest to return keyManager.addresses();
	virtual dev::AddressHash realAccounts() const override;
	// use web3 to submit a signed transaction to accept
	virtual eth::TransactionNotification authenticate(eth::TransactionSkeleton const& _t) override;

	void timerEvent(QTimerEvent*) override;

private:
	std::pair<bool, std::string> getUserNotice(eth::TransactionSkeleton const& _t);
	bool showAuthenticationPopup(std::string const& _title, std::string const& _text);
	bool validateTransaction(eth::TransactionSkeleton const& _t, bool _toProxy);

	bool m_isEnabled = true;

	std::queue<std::pair<eth::TransactionSkeleton, unsigned>> m_queued;
	std::unordered_map<unsigned, eth::TransactionNotification> m_queueOutput;
	unsigned m_nextQueueID = 0;
	Mutex x_queued;
	std::condition_variable m_queueCondition;

	AlethFace* m_aleth;
};

}
}
