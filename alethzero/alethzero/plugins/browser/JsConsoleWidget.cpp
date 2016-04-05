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
/** @file JsConsoleWidget.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "JsConsoleWidget.h"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <QWebEngineView>
#include <QScrollBar>
#include <QKeyEvent>
#include <libaleth/Common.h>
#include "WebPage.h"
#include "ui_JsConsoleWidget.h"
using namespace std;
using namespace dev;
using namespace aleth;

JsConsoleWidget::JsConsoleWidget(QWidget* _p):
	QWidget(_p),
	m_ui(new Ui::JsConsoleWidget())
{
	m_ui->setupUi(this);
	m_ui->jsInput->setAlignment(Qt::AlignBottom);
	m_ui->jsInput->installEventFilter(this);
}

JsConsoleWidget::~JsConsoleWidget()
{
}

void JsConsoleWidget::setWebView(QWebEngineView* _webView)
{
	WebPage* webPage = new WebPage(this); //install proxy web page
	_webView->setPage(webPage);
	connect(webPage, &WebPage::consoleMessage, [this](QString const& _msg) { JsConsoleWidget::addConsoleMessage(_msg, QString()); });
	connect(m_ui->jsInput, &QLineEdit::returnPressed, this, &JsConsoleWidget::execConsoleCommand);
	m_webView = _webView;
}

void JsConsoleWidget::eval(QString const& _js)
{
	if (_js.trimmed().isEmpty())
		return;
	auto f = [=](QVariant ev) {
		auto f2 = [=](QVariant jsonEv) {
			QString s;
			if (ev.isNull())
				s = "<span style=\"color: #888\">null</span>";
			else if (ev.type() == QVariant::String)
				s = "<span style=\"color: #444\">\"</span><span style=\"color: #c00\">" + ev.toString().toHtmlEscaped() + "</span><span style=\"color: #444\">\"</span>";
			else if (ev.type() == QVariant::Int || ev.type() == QVariant::Double)
				s = "<span style=\"color: #00c\">" + ev.toString().toHtmlEscaped() + "</span>";
			else if (jsonEv.type() == QVariant::String)
				s = "<span style=\"color: #840\">" + jsonEv.toString().toHtmlEscaped() + "</span>";
			else
				s = "<span style=\"color: #888\">unknown type</span>";
			addConsoleMessage(_js, s);
		};
		m_webView->page()->runJavaScript("JSON.stringify(___RET)", f2);
	};
	auto c = (_js.startsWith("{") || _js.startsWith("if ") || _js.startsWith("if(")) ? _js : ("___RET=(" + _js + ")");
	m_webView->page()->runJavaScript(c, f);
}

void JsConsoleWidget::addConsoleMessage(QString const& _js, QString const& _s)
{
	m_consoleHistory.push_back(qMakePair(_js, _s));
	QString r = "<html><body style=\"margin: 0;\">" ETH_HTML_DIV(ETH_HTML_MONO "position: absolute; bottom: 0; border: 0px; margin: 0px; width: 100%");
	for (auto const& i: m_consoleHistory)
		r +=	"<div style=\"border-bottom: 1 solid #eee; width: 100%\"><span style=\"float: left; width: 1em; color: #888; font-weight: bold\">&gt;</span><span style=\"color: #35d\">" + i.first.toHtmlEscaped() + "</span></div>"
				"<div style=\"border-bottom: 1 solid #eee; width: 100%\"><span style=\"float: left; width: 1em\">&nbsp;</span><span>" + i.second + "</span></div>";
	r += "</div></body></html>";
	m_ui->jsConsole->setHtml(r);
	m_ui->jsConsole->verticalScrollBar()->setSliderPosition(m_ui->jsConsole->verticalScrollBar()->maximum());
}

void JsConsoleWidget::execConsoleCommand()
{
	if (!m_ui->jsInput->text().isEmpty())
	{
		m_inputHistory.push_back(m_ui->jsInput->text());
		eval(m_ui->jsInput->text());
		m_ui->jsInput->setText("");
		m_historyIndex = m_inputHistory.size();
	}
}

bool JsConsoleWidget::eventFilter(QObject* _obj, QEvent* _event)
{
	(void)_obj;
	if (_event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
		if (keyEvent->key() == Qt::Key_Up)
		{
			if (m_historyIndex > 0 && !m_inputHistory.empty())
			{
				m_historyIndex--;
				m_ui->jsInput->setText(m_inputHistory[m_historyIndex]);
			}
			return true;
		}
		else if(keyEvent->key() == Qt::Key_Down)
		{
			if (m_historyIndex + 1 < m_inputHistory.size())
			{
				m_historyIndex++;
				m_ui->jsInput->setText(m_inputHistory[m_historyIndex]);
			}
			return true;
		}
	}
	return false;
}

