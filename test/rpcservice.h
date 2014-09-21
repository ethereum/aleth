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
/** @file rpcservice.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include <libwebthree/NetService.h>
#include "rpcprotocol.h"

namespace dev
{
	class DirectInterface
	{
	public:
		std::string a() { return "a"; }
		std::string b() { return "b"; }
		std::string c() { return "c"; }
	};
	
	class RPCService: public NetService<25, RPCService>
	{
	public:
		RPCService(): m_directInterface() {}
		
		std::string a() { return m_directInterface.a(); }
		std::string b() { return m_directInterface.b(); }
		std::string c() { return m_directInterface.c(); }
		
	protected:
		DirectInterface m_directInterface;
	};
}