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
/** @file GasPricing.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "GasPricing.h"
#include <QSettings>
#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include <libaleth/AlethFace.h>
#include "ZeroFace.h"
#include "ui_GasPricing.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;
using namespace zero;

ZERO_NOTE_PLUGIN(GasPricing);

GasPricing::GasPricing(ZeroFace* _m):
	Plugin(_m, "GasPricing")
{
	_m->addSettingsPage(2, "Gas Prices", [this]()
	{
		GasPricingPage* page = new GasPricingPage();
		connect(page, &SettingsPage::displayed, [this, page]() { page->setPrices(bid(), ask()); });
		connect(page, &SettingsPage::applied, [this, page]() { setPrices(page->bid(), page->ask()); });
		return page;
	});
}

u256 GasPricing::bid() const
{
	return static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->bid();
}

u256 GasPricing::ask() const
{
	return static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->ask();
}

void GasPricing::setPrices(u256 const& _bid, u256 const& _ask)
{
	static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->setBid(_bid);
	static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->setAsk(_ask);
}

void GasPricing::readSettings(QSettings const& _s)
{
	static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->setAsk(u256(_s.value("askPrice", QString::fromStdString(toString(DefaultGasPrice))).toString().toStdString()));
	static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->setBid(u256(_s.value("bidPrice", QString::fromStdString(toString(DefaultGasPrice))).toString().toStdString()));
}

void GasPricing::writeSettings(QSettings& _s)
{
	_s.setValue("askPrice", QString::fromStdString(toString(static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->ask())));
	_s.setValue("bidPrice", QString::fromStdString(toString(static_cast<TrivialGasPricer*>(aleth()->ethereum()->gasPricer().get())->bid())));
}

GasPricingPage::GasPricingPage():
	m_ui(new Ui::GasPricing())
{
	m_ui->setupUi(this);
}

u256 GasPricingPage::bid() const
{
	return fromValueUnits(m_ui->bidUnits, m_ui->bidValue);
}

u256 GasPricingPage::ask() const
{
	return fromValueUnits(m_ui->askUnits, m_ui->askValue);
}

void GasPricingPage::setPrices(u256 const& _bid, u256 const& _ask)
{
	setValueUnits(m_ui->bidUnits, m_ui->bidValue, _bid);
	setValueUnits(m_ui->askUnits, m_ui->askValue, _ask);
}

