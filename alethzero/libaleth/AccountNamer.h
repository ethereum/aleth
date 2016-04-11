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
/** @file AccountNamer.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <libethcore/Common.h>

namespace dev
{
namespace aleth
{

class AlethFace;

class AccountNamer
{
	friend class Aleth;

public:
	virtual std::string toName(Address const&) const { return std::string(); }
	virtual Address toAddress(std::string const&) const { return Address(); }
	virtual Addresses knownAddresses() const { return Addresses(); }

protected:
	void noteKnownChanged();
	void noteNamesChanged();

private:
	AlethFace* m_aleth = nullptr;
};

}
}
