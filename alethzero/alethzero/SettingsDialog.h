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
/** @file SettingDialog.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <functional>
#include <QDialog>

class QSettings;

namespace Ui { class SettingsDialog; }

namespace dev
{
namespace aleth
{
namespace zero
{

struct Category;
class CategoryModel;
class SettingsPage;
class MainFace;

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	void addPage(int _index, QString const& _title, std::function<SettingsPage*()> const& _factory);
	SettingsDialog(QWidget* _parent);
	~SettingsDialog();
	int exec() override;

signals:
	void applied();
	void displayed();

private:
	void apply();
	void accept() override;
	void reject() override;
	void currentChanged(QModelIndex const& _current);
	void showCategory(int _index);

	CategoryModel* m_model;
	Ui::SettingsDialog* m_ui;
	int m_currentCategory = 0;
};

}
}
}
