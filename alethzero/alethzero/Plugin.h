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

#pragma once

#include <QtWidgets/QAction>
#include <QtWidgets/QDockWidget>
#include <libdevcore/Common.h>

class QSettings;

namespace dev
{

class WebThreeDirect;
namespace eth { class Client; }
namespace shh { class WhisperHost; }

namespace aleth
{

class AlethFace;

namespace zero
{

class ZeroFace;

#define ZERO_NOTE_PLUGIN(ClassName) static DEV_UNUSED ::dev::aleth::zero::PluginRegistrar<ClassName> s_notePlugin(#ClassName)

class Plugin
{
public:
	Plugin(ZeroFace* _f, std::string const& _name);
	virtual ~Plugin() {}

	std::string const& name() const { return m_name; }

	ZeroFace* zero() const { return m_zero; }
	QDockWidget* dock(Qt::DockWidgetArea _area = Qt::RightDockWidgetArea, QString _title = QString());
	void addToDock(Qt::DockWidgetArea _area, QDockWidget* _dockwidget, Qt::Orientation _orientation);
	void addAction(QAction* _a);
	QMenu* ensureMenu(QString _menuName, QString _menuTitle);
	QAction* addMenuItem(QString _name, QString _menuName, bool _separate = false, QString _menuTitle = QString());
	QAction* addSeparator(QString _name, QString _menuName, QString _menuTitle = QString());

	AlethFace* aleth() const;
	dev::WebThreeDirect* web3() const;
	dev::eth::Client* ethereum() const;

	virtual void onAllChange() {}
	virtual void readSettings(QSettings const&) {}
	virtual void writeSettings(QSettings&) {}

	void noteSettingsChanged();

private:
	ZeroFace* m_zero = nullptr;
	std::string m_name;
	QDockWidget* m_dock = nullptr;
};

class SettingsPage: public QWidget
{
	Q_OBJECT
signals:
	void displayed();
	void applied();
};

class PluginRegistrarBase
{
public:
	PluginRegistrarBase(std::string const& _name, std::function<Plugin*(ZeroFace*)> const& _f);
	~PluginRegistrarBase();

private:
	std::string m_name;
};

template<class ClassName>
class PluginRegistrar: public PluginRegistrarBase
{
public:
	PluginRegistrar(std::string const& _name): PluginRegistrarBase(_name, [](ZeroFace* m){ return new ClassName(m); }) {}
};

}
}
}
