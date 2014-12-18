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
/** @file Extension.cpp
 * @author Yann yann@ethdev.com
 * @date 2014
 * Ethereum IDE client.
 */

#include <QMessageBox>
#include <QDebug>
#include <libevm/VM.h>
#include "Extension.h"
#include "AppContext.h"
using namespace dev;
using namespace dev::mix;

Extension::Extension()
{
	init();
}

Extension::Extension(ExtensionDisplayBehavior _displayBehavior)
{
	init();
	m_displayBehavior = _displayBehavior;
}

void Extension::init()
{
	m_ctx = AppContext::getInstance();
	m_appEngine = m_ctx->appEngine();
}

void Extension::addTabOn(QObject* _view)
{
	if (contentUrl() == "")
		return;

	QVariant returnValue;
	QQmlComponent* component = new QQmlComponent(
				AppContext::getInstance()->appEngine(),
				QUrl(contentUrl()), _view);

	QMetaObject::invokeMethod(_view, "addTab",
							Q_RETURN_ARG(QVariant, returnValue),
							Q_ARG(QVariant, this->title()),
							Q_ARG(QVariant, QVariant::fromValue(component)));

	m_view = qvariant_cast<QObject*>(returnValue);
}

void Extension::addContentOn(QObject* _view)
{
	Q_UNUSED(_view);
	if (m_displayBehavior == ExtensionDisplayBehavior::ModalDialog)
	{
		QObject* dialogWin = AppContext::getInstance()->appEngine()->rootObjects().at(0)->findChild<QObject*>("dialog", Qt::FindChildrenRecursively);
		QObject* dialogWinComponent = AppContext::getInstance()->appEngine()->rootObjects().at(0)->findChild<QObject*>("modalDialogContent", Qt::FindChildrenRecursively);
		QMetaObject::invokeMethod(dialogWin, "close");
		dialogWinComponent->setProperty("source", contentUrl());
		dialogWin->setProperty("title", title());
		QMetaObject::invokeMethod(dialogWin, "open");
	}
	//TODO add more view type.
}

