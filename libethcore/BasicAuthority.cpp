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
/** @file BasicAuthority.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "BasicAuthority.h"
#include <libdevcore/CommonJS.h>
#include <libdevcore/Log.h>
#include <libethereum/Interface.h>
#include "Exceptions.h"
#include "BlockHeader.h"
using namespace std;
using namespace dev;
using namespace eth;


void BasicAuthority::init()
{
	ETH_REGISTER_SEAL_ENGINE(BasicAuthority);
}

StringHashMap BasicAuthority::jsInfo(BlockHeader const& _bi) const
{
	return { { "sig", toJS(sig(_bi)) } };
}

bool BasicAuthority::shouldSeal(Interface* _i)
{
//	cdebug << "Comparing: " << _i->pendingInfo().timestamp() << " to " << utcTime();
	return _i->pendingInfo().timestamp() + 5 <= utcTime() || (_i->pendingInfo().timestamp() <= utcTime() && !_i->pending().empty());
}

void BasicAuthority::generateSeal(BlockHeader const& _bi)
{
	BlockHeader bi = _bi;
	h256 h = bi.hash(WithoutSeal);
	Signature s = sign(m_secret, h);
	setSig(bi, s);
	SealEngineBase::generateSeal(bi);
}

bool BasicAuthority::onOptionChanging(std::string const& _name, bytes const& _value)
{
	RLP rlp(_value);
	if (_name == "authorities")
		m_authorities = rlp.toUnorderedSet<Address>();
	else if (_name == "authority")
		m_secret = Secret(rlp.toHash<h256>());
	else
		return false;
	return true;
}

void BasicAuthority::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
	SealEngineFace::populateFromParent(_bi, _parent);
	// pseudo-random difficulty to facilitate fork reduction.
	_bi.setDifficulty(fromBigEndian<uint32_t>(sha3(sha3(m_secret) ^ _bi.parentHash()).ref().cropped(0, 4)));
}

void BasicAuthority::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
	SealEngineFace::verify(_s, _bi, _parent, _block);
	// check it hashes according to proof of work or that it's the genesis block.
	Signature s = sig(_bi);
	h256 h = _bi.hash(WithoutSeal);
	Address a = toAddress(recover(s, h));
	if (_s == CheckEverything && _bi.parentHash() && !m_authorities.count(a))
	{
		InvalidBlockNonce ex;
		ex << errinfo_hash256(_bi.hash(WithoutSeal));
		BOOST_THROW_EXCEPTION(ex);
	}
	else if (_s == QuickNonce && _bi.parentHash() && !SignatureStruct(sig(_bi)).isValid())
	{
		InvalidBlockNonce ex;
		ex << errinfo_hash256(_bi.hash(WithoutSeal));
		BOOST_THROW_EXCEPTION(ex);
	}
}
