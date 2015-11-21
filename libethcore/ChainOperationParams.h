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
/** @file ChainOperationsParams.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <libdevcore/Common.h>
#include "Common.h"

namespace dev
{
namespace eth
{

class PrecompiledContract
{
public:
	PrecompiledContract() = default;
	PrecompiledContract(std::function<bigint(unsigned)> const& _cost, std::function<void(bytesConstRef, bytesRef)> const& _exec):
		m_cost(_cost),
		m_execute(_exec)
	{}
	PrecompiledContract(unsigned _base, unsigned _word, std::function<void(bytesConstRef, bytesRef)> const& _exec);

	bigint cost(bytesConstRef _in) const { return m_cost(_in.size()); }
	void execute(bytesConstRef _in, bytesRef _out) const { m_execute(_in, _out); }

private:
	std::function<bigint(unsigned)> m_cost;
	std::function<void(bytesConstRef, bytesRef)> m_execute;
};

struct EVMSchedule
{
	bool exceptionalFailedCodeDeposit = true;
	u256 stackLimit = 1024;
	std::array<u256, 8> tierStepGas = {{0, 2, 3, 5, 8, 10, 20, 0}};
	u256 expGas = 10;
	u256 expByteGas = 10;
	u256 sha3Gas = 30;
	u256 sha3WordGas = 6;
	u256 sloadGas = 50;
	u256 sstoreSetGas = 20000;
	u256 sstoreResetGas = 5000;
	u256 sstoreRefundGas = 15000;
	u256 jumpdestGas = 1;
	u256 logGas = 375;
	u256 logDataGas = 8;
	u256 logTopicGas = 375;
	u256 createGas = 32000;
	u256 callGas = 40;
	u256 callStipend = 2300;
	u256 callValueTransferGas = 9000;
	u256 callNewAccountGas = 25000;
	u256 suicideRefundGas = 24000;
	u256 memoryGas = 3;
	u256 quadCoeffDiv = 512;
	u256 createDataGas = 200;
	u256 txGas = 53000;
	u256 txDataZeroGas = 4;
	u256 txDataNonZeroGas = 68;
	u256 copyGas = 3;
};

struct ChainOperationParams
{
	ChainOperationParams();

	explicit operator bool() const { return accountStartNonce != Invalid256; }

	/// The chain sealer name: e.g. Ethash, NoProof, BasicAuthority
	std::string sealEngineName = "NoProof";

	/// General chain params.
	u256 blockReward = 0;
	u256 maximumExtraDataSize = 1024;
	u256 accountStartNonce = 0;
	EVMSchedule evmSchedule;

	/// Precompiled contracts as specified in the chain params.
	std::unordered_map<Address, PrecompiledContract> precompiled;

	/**
	 * @brief Additional parameters.
	 *
	 * e.g. Ethash specific:
	 * - minGasLimit
	 * - gasLimitBoundDivisor
	 * - minimumDifficulty
	 * - difficultyBoundDivisor
	 * - durationLimit
	 */
	std::unordered_map<std::string, std::string> otherParams;

	/// Convenience method to get an otherParam as a u256 int.
	u256 u256Param(std::string const& _name) const;
};

}
}
