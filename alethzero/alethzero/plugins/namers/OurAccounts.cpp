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
/** @file OurAccounts.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "OurAccounts.h"
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include <libethcore/KeyManager.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(OurAccounts);

OurAccounts::OurAccounts(ZeroFace* _m):
	AccountNamerPlugin(_m, "OurAccounts")
{
	connect(aleth(), SIGNAL(keysChanged()), SLOT(updateNames()));
	updateNames();
}

OurAccounts::~OurAccounts()
{
}

std::string OurAccounts::toName(Address const& _a) const
{
	return aleth()->keyManager().accountName(_a);
}

Address OurAccounts::toAddress(std::string const& _n) const
{
	if (m_names.count(_n))
		return m_names.at(_n);
	return Address();
}

Addresses OurAccounts::knownAddresses() const
{
	return aleth()->keyManager().accounts();
}

void OurAccounts::updateNames()
{
	m_names.clear();
	for (Address const& i: aleth()->keyManager().accounts())
		m_names[aleth()->keyManager().accountName(i)] = i;
	noteKnownChanged();
}
