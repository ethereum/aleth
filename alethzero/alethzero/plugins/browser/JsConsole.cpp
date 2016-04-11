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
/** @file JsConsole.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "JsConsole.h"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <QWebEngineView>
#include <libaleth/AlethWhisper.h>
#include <libaleth/AlethFace.h>
#include <libaleth/DappLoader.h>
#include <libweb3jsonrpc/SessionManager.h>
#include "JsConsoleWidget.h"
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(JsConsole);

namespace dev { namespace aleth { QString contentsOfQResource(std::string const& res); } }

JsConsole::JsConsole(ZeroFace* _m):
	Plugin(_m, "JavaScript Console")
{
	JsConsoleWidget* jsConsole = new JsConsoleWidget(_m);
	QWebEngineView* webView  = new QWebEngineView(jsConsole);
	webView->setVisible(false);
	jsConsole->setWebView(webView);
	dock(Qt::BottomDockWidgetArea, "JavaScript Console")->setWidget(jsConsole);
	std::string adminSessionKey = zero()->sessionManager()->newSession(rpc::SessionPermissions{{rpc::Privilege::Admin}});

	QString content = "<script>\n";
	content += DappLoader::makeJSCode();
	content += "\nweb3.admin.setSessionKey('" + QString::fromStdString(adminSessionKey) + "');";
	content += "</script>\n";
	webView->setHtml(content);
}

JsConsole::~JsConsole()
{
}

