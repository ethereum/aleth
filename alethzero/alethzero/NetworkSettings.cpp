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

/** @file NetworkSettings.cpp
 * @author Arkadiy Paronyan <arkadiy@ethdev.com>
 * @date 2015
 */
#include "NetworkSettings.h"
#include <QSettings>
#include <libp2p/Common.h>
#include "ui_NetworkSettings.h"

using namespace dev;
using namespace aleth;
using namespace zero;
using namespace eth;
using namespace std;

void NetworkSettings::read(QSettings const& _settings)
{
	p2pSettings.publicIPAddress = _settings.value("forceAddress", "").toString().toStdString();
	p2pSettings.listenIPAddress = _settings.value("listenIP", "").toString().toStdString();
	p2pSettings.listenPort =  _settings.value("port", p2p::c_defaultListenPort).toInt();
	p2pSettings.traverseNAT = _settings.value("upnp", true).toBool();
	clientName = _settings.value("clientName", "").toString();
	idealPeers = _settings.value("idealPeers", 50).toInt();
	hermitMode = _settings.value("hermitMode", false).toBool();
	dropPeers = _settings.value("dropPeers", false).toBool();
	p2pSettings.discovery = !hermitMode;
	p2pSettings.pin = !p2pSettings.discovery;
}

void NetworkSettings::write(QSettings& _settings) const
{
	_settings.setValue("forceAddress", QString::fromStdString(p2pSettings.publicIPAddress));
	_settings.setValue("listenIP", QString::fromStdString(p2pSettings.listenIPAddress));
	_settings.setValue("port", p2pSettings.listenPort);
	_settings.setValue("upnp", p2pSettings.traverseNAT);
	_settings.setValue("clientName", clientName);
	_settings.setValue("idealPeers", idealPeers);
	_settings.setValue("hermitMode", hermitMode);
	_settings.setValue("dropPeers", dropPeers);
}

NetworkSettingsPage::NetworkSettingsPage():
	m_ui(new Ui::NetworkSettings())
{
	m_ui->setupUi(this);
}

NetworkSettings NetworkSettingsPage::prefs() const
{
	auto listenIP = m_ui->listenIP->text().toStdString();
	boost::system::error_code ec;
	listenIP = bi::address::from_string(listenIP, ec).to_string();
	if (ec)
		listenIP.clear();

	auto publicIP = m_ui->forcePublicIP->text().toStdString();
	publicIP = bi::address::from_string(publicIP, ec).to_string();
	if (ec)
		publicIP.clear();

	NetworkSettings ret;

	if (p2p::isPublicAddress(publicIP))
		ret.p2pSettings = p2p::NetworkPreferences(publicIP, listenIP, m_ui->port->value(), m_ui->useUpnp->isChecked());
	else
		ret.p2pSettings = p2p::NetworkPreferences(listenIP, m_ui->port->value(), m_ui->useUpnp->isChecked());

	ret.clientName = m_ui->clientName->text();
	ret.idealPeers = m_ui->idealPeers->value();
	ret.hermitMode = m_ui->hermitMode->isChecked();
	ret.dropPeers = m_ui->dropPeers->isChecked();
	return ret;
}


void NetworkSettingsPage::setPrefs(NetworkSettings const& _settings)
{
	m_ui->forcePublicIP->setText(QString::fromStdString(_settings.p2pSettings.publicIPAddress));
	m_ui->port->setValue(_settings.p2pSettings.listenPort);
	m_ui->listenIP->setText(QString::fromStdString(_settings.p2pSettings.listenIPAddress));
	m_ui->clientName->setText(_settings.clientName);
	m_ui->idealPeers->setValue(_settings.idealPeers);
	m_ui->hermitMode->setChecked(_settings.hermitMode);
	m_ui->useUpnp->setChecked(_settings.p2pSettings.traverseNAT);
	m_ui->dropPeers->setChecked(_settings.dropPeers);
}
