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
/** @file PrivateChain.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "PrivateChain.h"
#include <QInputDialog>
#include <QSettings>
#include <libethereum/Client.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(PrivateChain);

PrivateChain::PrivateChain(ZeroFace* _z):
	Plugin(_z, "PrivateChain")
{
	QAction* a = addMenuItem("Use Private Chain...", "menuConfig", true);
	a->setObjectName("usePrivate");
	a->setCheckable(true);
	connect(a, SIGNAL(triggered(bool)), SLOT(usePrivate(bool)));
}

void PrivateChain::usePrivate(bool _enable)
{
	QString pc;
	if (_enable)
	{
		bool ok;
		pc = QInputDialog::getText(zero(), "Enter Name", "Enter the name of your private chain", QLineEdit::Normal, QString("NewChain-%1").arg(time(0)), &ok);
		if (!ok)
		{
			zero()->findChild<QAction*>("usePrivate")->setChecked(false);
			return;
		}
	}
	setPrivateChain(pc);
}

void PrivateChain::setPrivateChain(QString const& _private, bool _forceConfigure)
{
	if (m_id == _private && !_forceConfigure)
		return;

	// Prep UI bits.
	zero()->allStop();

	m_id = _private;

	ChainParams p = aleth()->baseParams();
	if (!m_id.isEmpty())
	{
		p.extraData = sha3(m_id.toStdString()).asBytes();
		p.difficulty = p.u256Param("minimumDifficulty");
		p.gasLimit = u256(1) << 32;
		aleth()->reopenChain(p);
	}
	else
		aleth()->reopenChain(p);
	zero()->findChild<QAction*>("usePrivate")->setChecked(!m_id.isEmpty());
	zero()->writeSettings();

	// Rejig UI bits.
	zero()->carryOn();
}

void PrivateChain::readSettings(QSettings const& _s)
{
	setPrivateChain(_s.value("privateChain", "").toString());
}

void PrivateChain::writeSettings(QSettings& _s)
{
	_s.setValue("privateChain", m_id);
}
