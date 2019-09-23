// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <algorithm>
#include "Common.h"

namespace dev
{
namespace p2p
{

struct RLPXInvalidPacket: virtual dev::Exception {};

static bytesConstRef nextRLP(bytesConstRef _b) { try { RLP r(_b, RLP::AllowNonCanon); return _b.cropped(0, std::min((size_t)r.actualSize(), _b.size())); } catch(...) {} return bytesConstRef(); }

/**
 * RLPX Packet
 */
class RLPXPacket
{
public:
	/// Construct packet. RLPStream data is invalidated.
	RLPXPacket(uint8_t _capId, RLPStream& _type, RLPStream& _data): m_cap(_capId), m_type(_type.out()), m_data(_data.out()) {}

	/// Construct packet from single bytestream. RLPStream data is invalidated.
	RLPXPacket(uint8_t _capId, bytesConstRef _in): m_cap(_capId), m_type(nextRLP(_in).toBytes()) { if (_in.size() > m_type.size()) { m_data.resize(_in.size() - m_type.size()); _in.cropped(m_type.size()).copyTo(&m_data); } }
	
	RLPXPacket(RLPXPacket const& _p) = delete;
	RLPXPacket(RLPXPacket&& _p): m_cap(_p.m_cap), m_type(_p.m_type), m_data(_p.m_data) {}

	bytes const& type() const { return m_type; }

	bytes const& data() const { return m_data; }

	uint8_t cap() const { return m_cap; }
	
	size_t size() const { try { return RLP(m_type).actualSize() + RLP(m_data, RLP::LaissezFaire).actualSize(); } catch(...) { return 0; } }

	/// Appends byte data and returns if packet is valid.
	bool append(bytesConstRef _in) { auto offset = m_data.size(); m_data.resize(offset + _in.size()); _in.copyTo(bytesRef(&m_data).cropped(offset)); return isValid(); }
	
	virtual bool isValid() const noexcept { try { return !(m_type.empty() && m_data.empty()) && RLP(m_type).actualSize() == m_type.size() && RLP(m_data).actualSize() == m_data.size(); } catch (...) {} return false; }
	
protected:
	uint8_t m_cap;
	bytes m_type;
	bytes m_data;
};

}
}