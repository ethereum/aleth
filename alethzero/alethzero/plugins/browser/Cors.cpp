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
/** @file Cors.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "Cors.h"
#include <QSettings>
#include <libweb3jsonrpc/SafeHttpServer.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
#include "ui_Cors.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(Cors);

Cors::Cors(ZeroFace* _m):
	Plugin(_m, "Cors")
{
	_m->addSettingsPage(2, "Security", [this](){
		CorsSettingsPage* page = new CorsSettingsPage();
		connect(page, &SettingsPage::displayed, [this, page]() { page->setDomain(QString::fromStdString(zero()->web3ServerConnector()->allowedOrigin())); });
		connect(page, &SettingsPage::applied, [this, page]() { setDomain(page->domain()); });
		return page;
	});
}

Cors::~Cors()
{
}

void Cors::readSettings(QSettings const& _s)
{
	setDomain(_s.value("corsDomain", "").toString());
}

void Cors::writeSettings(QSettings& _s)
{
	_s.setValue("corsDomain", QString::fromStdString(zero()->web3ServerConnector()->allowedOrigin()));
}

void Cors::setDomain(QString const& _domain)
{
	zero()->web3ServerConnector()->setAllowedOrigin(_domain.toStdString());
}

CorsSettingsPage::CorsSettingsPage():
	m_ui(new Ui::Cors())
{
	m_ui->setupUi(this);
}

void CorsSettingsPage::setDomain(QString const& _domain)
{
	if (_domain == "")
		m_ui->disallow->setChecked(true);
	else if (_domain == "*")
		m_ui->allow->setChecked(true);
	else
	{
		m_ui->special->setChecked(true);
		m_ui->domain->setText(_domain);
	}
}

QString CorsSettingsPage::domain() const
{
	QString domain;
	if (m_ui->allow->isChecked())
		domain = "*";
	else if (m_ui->disallow->isChecked())
		domain = "";
	else
		domain = m_ui->domain->text();
	return domain;
}
