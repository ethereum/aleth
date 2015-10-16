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
/** @file Ethash.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * A proof of work algorithm.
 */

#pragma once

#include "EthashProofOfWork.h"
#include "GenericFarm.h"
#include "Sealer.h"

namespace dev
{

namespace eth
{

class Ethash: public SealEngineFace
{
public:
	Ethash();

	std::string name() const override { return "Ethash"; }
	unsigned revision() const override { return 1; }
	unsigned sealFields() const override { return 2; }
	bytes sealRLP() const override { return rlp(h256()) + rlp(Nonce()); }

	StringHashMap jsInfo(BlockInfo const& _bi) const override;
	void verify(Strictness _s, BlockInfo const& _bi, BlockInfo const& _parent, bytesConstRef _block) const override;
	void populateFromParent(BlockInfo& _bi, BlockInfo const& _parent) const override;

	strings sealers() const override;
	std::string sealer() const override { return m_sealer; }
	void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
	void cancelGeneration() override { m_farm.stop(); }
	void generateSeal(BlockInfo const& _bi) override;
	void onSealGenerated(std::function<void(bytes const&)> const& _f) override;

	eth::GenericFarm<EthashProofOfWork>& farm() { return m_farm; }

	enum { MixHashField = 0, NonceField = 1 };
	static h256 seedHash(BlockInfo const& _bi);
	static Nonce nonce(BlockInfo const& _bi) { return _bi.seal<Nonce>(NonceField); }
	static h256 mixHash(BlockInfo const& _bi) { return _bi.seal<h256>(MixHashField); }
	static h256 boundary(BlockInfo const& _bi) { auto d = _bi.difficulty(); return d ? (h256)u256((bigint(1) << 256) / d) : h256(); }
	static BlockInfo& setNonce(BlockInfo& _bi, Nonce _v) { _bi.setSeal(NonceField, _v); return _bi; }
	static BlockInfo& setMixHash(BlockInfo& _bi, h256 const& _v) { _bi.setSeal(MixHashField, _v); return _bi; }

	u256 calculateDifficulty(BlockInfo const& _bi, BlockInfo const& _parent) const;
	u256 childGasLimit(BlockInfo const& _bi, u256 const& _gasFloorTarget = Invalid256) const;

	void manuallySetWork(BlockInfo const& _work) { m_sealing = _work; }
	void manuallySubmitWork(h256 const& _mixHash, Nonce _nonce);

	static void ensurePrecomputed(unsigned _number);

private:
	bool verifySeal(BlockInfo const& _bi) const;
	bool quickVerifySeal(BlockInfo const& _bi) const;

	eth::GenericFarm<EthashProofOfWork> m_farm;
	std::string m_sealer = "cpu";
	BlockInfo m_sealing;
};

}
}
