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
/** @file FixedClient.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "FixedClient.h"

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

eth::State FixedClient::asOf(BlockNumber _h) const
{
	ReadGuard l(x_stateDB);
	if (_h == PendingBlock || _h == LatestBlock)
		return m_state;
	
	return State(m_state.db(), bc(), bc().numberHash(_h));
}

eth::State FixedClient::asOf(h256 _h) const
{
	ReadGuard l(x_stateDB);
	return State(m_state.db(), bc(), _h);
}
