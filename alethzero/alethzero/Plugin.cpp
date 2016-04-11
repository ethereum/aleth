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
/** @file Plugin.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Plugin.h"
#include <QMenu>
#include <QMenuBar>
#include <libwebthree/WebThree.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

Plugin::Plugin(ZeroFace* _f, std::string const& _name):
	m_zero(_f),
	m_name(_name)
{
	_f->adoptPlugin(this);
}

AlethFace* Plugin::aleth() const
{
	return m_zero->aleth();
}

WebThreeDirect* Plugin::web3() const
{
	return aleth()->web3();
}

eth::Client* Plugin::ethereum() const
{
	return web3()->ethereum();
}

QDockWidget* Plugin::dock(Qt::DockWidgetArea _area, QString _title)
{
	if (_title.isEmpty())
		_title = QString::fromStdString(m_name);
	if (!m_dock)
	{
		m_dock = new QDockWidget(_title, zero());
		m_dock->setObjectName(_title);
		zero()->addDockWidget(_area, m_dock);
		m_dock->setFeatures(QDockWidget::AllDockWidgetFeatures | QDockWidget::DockWidgetVerticalTitleBar);
	}
	return m_dock;
}

void Plugin::addToDock(Qt::DockWidgetArea _area, QDockWidget* _dockwidget, Qt::Orientation _orientation)
{
	zero()->addDockWidget(_area, _dockwidget, _orientation);
}

void Plugin::addAction(QAction* _a)
{
	zero()->addAction(_a);
}

QMenu* Plugin::ensureMenu(QString _menuName, QString _menuTitle)
{
	QMenu* m = zero()->findChild<QMenu*>(_menuName);
	if (!m)
	{
		m = zero()->menuBar()->addMenu(_menuTitle);
		m->setObjectName(_menuName);
	}
	return m;
}

QAction* Plugin::addMenuItem(QString _n, QString _menuName, bool _sep, QString _menuTitle)
{
	QMenu* m = ensureMenu(_menuName, _menuTitle);
	if (_sep)
		m->addSeparator();
	QAction* a = new QAction(_n, zero());
	m->addAction(a);
	return a;
}

QAction* Plugin::addSeparator(QString _n, QString _menuName, QString _menuTitle)
{
	QMenu* m = ensureMenu(_menuName, _menuTitle);
	QAction* a = m->addSeparator();
	a->setText(_n);
	return a;
}

void Plugin::noteSettingsChanged()
{
	m_zero->writeSettings();
}

PluginRegistrarBase::PluginRegistrarBase(std::string const& _name, PluginFactory const& _f):
	m_name(_name)
{
//	cdebug << "Noting linked plugin" << _name;
	ZeroFace::notePlugin(_name, _f);
}

PluginRegistrarBase::~PluginRegistrarBase()
{
//	cdebug << "Noting unlinked plugin" << m_name;
	ZeroFace::unnotePlugin(m_name);
}
