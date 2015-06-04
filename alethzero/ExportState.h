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
/** @file ExportState.h
 * @author Arkadiy Paronyan <arkadiy@ethdev.com>
 * @date 2015
 */

#pragma once

#include <memory>
#include <QDialog>
#include <libethcore/Common.h>

namespace Ui { class ExportState; }
namespace dev { namespace eth { class Client; } }

class Main;

class ExportStateDialog: public QDialog
{
	Q_OBJECT

public:
	explicit ExportStateDialog(Main* _parent = 0);
	virtual ~ExportStateDialog();

private slots:
	void on_block_editTextChanged();
	void on_block_currentIndexChanged(int _index);
	void on_saveButton_clicked();

private:
	dev::eth::Client* ethereum() const;
	void fillBlocks();
	void fillContracts();
	void generateJSON();

private:
	std::unique_ptr<Ui::ExportState> ui;
	Main* m_main;
	int m_recentBlocks = 0;
	dev::eth::BlockNumber m_block = dev::eth::LatestBlock;
};
