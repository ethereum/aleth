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
/** @file Transact.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Transact.h"
#include <QToolBar>
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include <libethcore/KeyManager.h>
#include <libaleth/AlethFace.h>
#include "TransactDialog.h"
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(Transact);

Transact::Transact(ZeroFace* _m):
	Plugin(_m, "Transact")
{
	m_dialog = new TransactDialog(_m, _m->aleth());
	m_dialog->setWindowFlags(Qt::Dialog);
	m_dialog->setWindowModality(Qt::WindowModal);

	QAction* a = addMenuItem("New &Transaction...", "menuActions", false, "A&ctions");
	connect(a, SIGNAL(triggered()), SLOT(newTransaction()));
	zero()->findChild<QToolBar*>()->addAction(a);
}

Transact::~Transact()
{
	delete m_dialog;
}

void Transact::newTransaction()
{
	m_dialog->setEnvironment(aleth()->keyManager().accountsHash(), ethereum(), &aleth()->natSpec());
	m_dialog->show();
}

