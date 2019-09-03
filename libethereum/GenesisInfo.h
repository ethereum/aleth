/*
        This file is part of aleth.

        aleth is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        aleth is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file GenesisInfo.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <libdevcore/Common.h>

#include <string>

namespace dev
{
namespace eth
{

extern std::string const c_genesisInfoTestBasicAuthority;
extern dev::Addresses childDaos();

}
}
