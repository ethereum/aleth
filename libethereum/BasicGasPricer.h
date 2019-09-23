// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <array>
#include "GasPricer.h"

namespace dev
{
namespace eth
{

class BasicGasPricer: public GasPricer
{
public:
	explicit BasicGasPricer(u256 _weiPerRef, u256 _refsPerBlock): m_weiPerRef(_weiPerRef), m_refsPerBlock(_refsPerBlock) {}

	void setRefPrice(u256 _weiPerRef) { if ((bigint)m_refsPerBlock * _weiPerRef > std::numeric_limits<u256>::max() ) BOOST_THROW_EXCEPTION(Overflow() << errinfo_comment("ether price * block fees is larger than 2**256-1, choose a smaller number.") ); else m_weiPerRef = _weiPerRef; }
	void setRefBlockFees(u256 _refsPerBlock) { if ((bigint)m_weiPerRef * _refsPerBlock > std::numeric_limits<u256>::max() ) BOOST_THROW_EXCEPTION(Overflow() << errinfo_comment("ether price * block fees is larger than 2**256-1, choose a smaller number.") ); else m_refsPerBlock = _refsPerBlock; }

	u256 ask(Block const&) const override { return m_weiPerRef * m_refsPerBlock / m_gasPerBlock; }
	u256 bid(TransactionPriority _p = TransactionPriority::Medium) const override { return m_octiles[(int)_p] > 0 ? m_octiles[(int)_p] : (m_weiPerRef * m_refsPerBlock / m_gasPerBlock); }

	void update(BlockChain const& _bc) override;

private:
	u256 m_weiPerRef;
	u256 m_refsPerBlock;
	u256 m_gasPerBlock = DefaultBlockGasLimit;
	std::array<u256, 9> m_octiles;
};

}
}
