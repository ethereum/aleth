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
/** @file SpecialBlockChain.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "SpecialBlockChain.h"
#include <QInputDialog>
#include <libethereum/Client.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(SpecialBlockChain);

SpecialBlockChain::SpecialBlockChain(ZeroFace* _z):
	Plugin(_z, "SpecialBlockChain")
{
	addSeparator("Block Chain", "menuSpecial");
	connect(addMenuItem("Rewind", "menuSpecial"), SIGNAL(triggered()), SLOT(rewind()));
	connect(addMenuItem("Inject", "menuSpecial"), SIGNAL(triggered()), SLOT(inject()));
	connect(addMenuItem("Retry Unknown", "menuSpecial"), SIGNAL(triggered()), SLOT(retryUnknown()));
	connect(addMenuItem("Kill", "menuSpecial"), SIGNAL(triggered()), SLOT(kill()));
}

void SpecialBlockChain::rewind()
{
	bool ok;
	int n = QInputDialog::getInt(zero(), "Rewind Chain", "Enter the number of the new chain head.", ethereum()->number() * 9 / 10, 1, ethereum()->number(), 1, &ok);
	if (ok)
	{
		ethereum()->rewind(n);
		zero()->noteAllChange();
	}
}

void SpecialBlockChain::inject()
{
	QString s = QInputDialog::getText(zero(), "Inject Block", "Enter block dump in hex");
	try
	{
		bytes b = fromHex(s.toStdString(), WhenError::Throw);
		ethereum()->injectBlock(b);
	}
	catch (BadHexCharacter& _e)
	{
		cwarn << "invalid hex character, transaction rejected";
		cwarn << boost::diagnostic_information(_e);
	}
	catch (...)
	{
		cwarn << "block rejected";
	}
}

void SpecialBlockChain::retryUnknown()
{
	ethereum()->retryUnknown();
}

void SpecialBlockChain::kill()
{
	zero()->allStop();
	ethereum()->killChain();
	zero()->carryOn();
}

