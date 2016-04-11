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
/** @file BlockChain.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <QListWidget>
#include <QPlainTextEdit>
#include "Plugin.h"

namespace Ui
{
class BlockList;
}

namespace dev
{
namespace aleth
{
namespace zero
{

class BlockList: public QObject, public Plugin
{
	Q_OBJECT

public:
	BlockList(ZeroFace* _m);
	~BlockList();

private slots:
	void refreshBlocks();
	void refreshInfo();
	void debugCurrent();
	void filterChanged();

private:
	void dumpState(bool _post);

	void onAllChange() override { refreshBlocks(); }
	void readSettings(QSettings const&) override {}
	void writeSettings(QSettings&) override {}

	Ui::BlockList* m_ui;

	unsigned m_newBlockWatch;
	unsigned m_pendingWatch;
	bool m_inhibitInfoRefresh;
};

}
}
}
