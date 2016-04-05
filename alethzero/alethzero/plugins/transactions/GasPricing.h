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
/** @file GasPricing.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include "Plugin.h"

namespace Ui { class GasPricing; }

namespace dev
{
namespace aleth
{
namespace zero
{

class GasPricing: public QObject, public Plugin
{
	Q_OBJECT

public:
	GasPricing(ZeroFace* _m);

private:
	void setPrices(u256 const& _bid, u256 const& _ask);
	void readSettings(QSettings const&) override;
	void writeSettings(QSettings&) override;
	u256 bid() const;
	u256 ask() const;
};

class GasPricingPage: public SettingsPage
{
public:
	GasPricingPage();
	void setPrices(u256 const& _bid, u256 const& _ask);
	u256 bid() const;
	u256 ask() const;

private:
	Ui::GasPricing* m_ui;
};

}
}
}
