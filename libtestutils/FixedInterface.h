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
/** @file FixedInterface.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once

#include <libethereum/InterfaceStub.h>
#include <libethereum/BlockChain.h>
#include <libethereum/State.h>

namespace dev
{
namespace test
{

class FixedInterface: public dev::eth::InterfaceStub
{
public:
	FixedInterface(eth::BlockChain const& _bc, eth::State _state) :  m_bc(_bc), m_state(_state) {}
	virtual ~FixedInterface() {}
	
	// stub
	virtual void flushTransactions() override {}
	virtual eth::BlockChain const& bc() const override { return m_bc; }
	virtual eth::State asOf(eth::BlockNumber _h) const override;
	virtual eth::State asOf(h256 _h) const override;
	virtual eth::State preMine() const override { ReadGuard l(x_stateDB); return m_state; }
	virtual eth::State postMine() const override { ReadGuard l(x_stateDB); return m_state; }
	virtual void prepareForTransaction() override {}

private:
	eth::BlockChain const& m_bc;
	eth::State m_state;
	mutable SharedMutex x_stateDB;			///< Lock on the state DB, effectively a lock on m_postMine.
};

}
}
