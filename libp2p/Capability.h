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
/** @file Capability.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
#include "HostCapability.h"

namespace dev
{
namespace p2p
{

class ReputationManager;

class Capability: public std::enable_shared_from_this<Capability>
{
	friend class Session;

public:
	Capability(std::shared_ptr<SessionFace> _s, HostCapabilityFace* _h, unsigned _idOffset, uint16_t _protocolID);
	virtual ~Capability() {}

	// Implement these in the derived class.
/*	static std::string name() { return ""; }
	static u256 version() { return 0; }
	static unsigned messageCount() { return 0; }
*/
protected:
	std::shared_ptr<SessionFace> session() const { return m_session.lock(); }
	HostCapabilityFace* hostCapability() const { return m_hostCap; }

	virtual bool interpret(unsigned _id, RLP const&) = 0;

	void disable(std::string const& _problem);

	RLPStream& prep(RLPStream& _s, unsigned _id, unsigned _args = 0);
	void sealAndSend(RLPStream& _s);
	void addRating(int _r);

	uint16_t const c_protocolID;

private:
	std::weak_ptr<SessionFace> m_session;
	HostCapabilityFace* m_hostCap;
	bool m_enabled = true;
	unsigned m_idOffset;
};

}
}
