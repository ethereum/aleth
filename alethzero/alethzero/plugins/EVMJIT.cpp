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
/** @file EVMJIT.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "EVMJIT.h"
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <libevm/VMFactory.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"

using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(EVMJIT);

EVMJIT::EVMJIT(ZeroFace* _z):
	Plugin(_z, "EVMJIT")
{
	m_group = new QActionGroup(this);
	m_group->setExclusive(true);
	vector<pair<QString, VMKind>> kinds =
	{
		{ "Interpreter VM", VMKind::Interpreter },
		{ "JIT VM", VMKind::JIT },
		{ "Smart VM", VMKind::Smart }
	};
	addSeparator("VM Type", "menuSpecial");
	for (pair<QString, VMKind> const& k: kinds)
	{
		QAction* a = addMenuItem(k.first, "menuSpecial");
		a->setCheckable(true);
		connect(a, &QAction::triggered, [=]() { VMFactory::setKind(k.second); });
		m_group->addAction(a);

#if ETH_EVMJIT
		if (k.second == VMKind::Smart)
		{
			a->setChecked(true);
			VMFactory::setKind(k.second);
		}
#else
		a->setChecked(k.second == VMKind::Interpreter);
		a->setEnabled(k.second == VMKind::Interpreter);
#endif // ETH_EVMJIT
	}
}

void EVMJIT::readSettings(QSettings const& _s)
{
#if ETH_EVMJIT // We care only if JIT is enabled. Otherwise it can cause misconfiguration.
	auto vmName = _s.value("vm").toString();
	if (!vmName.isEmpty())
		for (QAction* a: m_group->actions())
			if (vmName == a->text())
			{
				a->setChecked(true);
				a->trigger();
			}
#else
	(void)_s;
#endif // ETH_EVMJIT
}

void EVMJIT::writeSettings(QSettings& _s)
{
	if (auto vm = m_group->checkedAction())
		_s.setValue("vm", vm->text());
}
