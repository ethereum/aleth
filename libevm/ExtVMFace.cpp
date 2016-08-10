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
/** @file ExtVMFace.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "ExtVMFace.h"
#include <libethcore/SealEngine.h>
using namespace dev;
using namespace dev::eth;

ExtVMFace::ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash, unsigned _depth):
	m_envInfo(_envInfo),
	myAddress(_myAddress),
	caller(_caller),
	origin(_origin),
	value(_value),
	gasPrice(_gasPrice),
	data(_data),
	code(std::move(_code)),
	codeHash(_codeHash),
	depth(_depth)
{}
