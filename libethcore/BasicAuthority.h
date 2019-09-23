// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcrypto/Common.h>
#include "SealEngine.h"

namespace dev
{
namespace eth
{

class BasicAuthority: public SealEngineBase
{
public:
    static std::string name() { return "BasicAuthority"; }
    unsigned revision() const override { return 0; }
	unsigned sealFields() const override { return 1; }
	bytes sealRLP() const override { return rlp(Signature()); }

	void populateFromParent(BlockHeader&, BlockHeader const&) const override;
	StringHashMap jsInfo(BlockHeader const& _bi) const override;
	void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
	bool shouldSeal(Interface*) override;
	void generateSeal(BlockHeader const& _bi) override;

	static Signature sig(BlockHeader const& _bi) { return _bi.seal<Signature>(); }
	static BlockHeader& setSig(BlockHeader& _bi, Signature const& _sig) { _bi.setSeal(_sig); return _bi; }
	void setSecret(Secret const& _s) { m_secret = _s; }
	static void init();

private:
	bool onOptionChanging(std::string const& _name, bytes const& _value) override;

	Secret m_secret;
	AddressHash m_authorities;
};

}
}
