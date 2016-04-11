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
/** @file Browser.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include "Plugin.h"

class QWebEnginePage;

namespace Ui { class Browser; }

namespace dev
{
namespace aleth
{

class DappLoader;
class DappHost;
struct Dapp;

namespace zero
{

class Browser: public QObject, public Plugin
{
	Q_OBJECT

public:
	Browser(ZeroFace* _m);
	~Browser();

private slots:
	void readSettings(QSettings const&) override;
	void writeSettings(QSettings&) override;
	void dappLoaded(Dapp& _dapp); //qt does not support rvalue refs for signals
	void pageLoaded(QByteArray const& _content, QString const& _mimeType, QUrl const& _uri);
	void reloadUrl();
	void loadJs();

private:
	void init();

	Ui::Browser* m_ui;
	std::unique_ptr<DappHost> m_dappHost;
	DappLoader* m_dappLoader = nullptr;
	bool m_finding = false;
};

}
}
}
