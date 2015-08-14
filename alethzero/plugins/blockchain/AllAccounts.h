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
/** @file AllAccounts.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#if ETH_FATDB

#include <QListWidget>
#include <QPlainTextEdit>
#include "MainFace.h"

namespace Ui
{
class AllAccounts;
}

namespace dev
{
namespace az
{

class AllAccounts: public QObject, public Plugin
{
	Q_OBJECT

public:
	AllAccounts(MainFace* _m);
	~AllAccounts();

private slots:
	void on_accounts_currentItemChanged();
	void on_accounts_doubleClicked();

	void onAllChange();
	void refresh();

private:
	void installWatches();

	Ui::AllAccounts* m_ui;
};

}
}

#endif
