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

struct ChainOperationParams
{
	ChainOperationParams(): accountStartNonce(Invalid256) {}

	explicit operator bool() const { return accountStartNonce != Invalid256; }

	/// The chain sealer name: e.g. Ethash, Fluidity, BasicAuthority
	std::string sealEngineName = "NoProof";

	/// Chain params.
	u256 blockReward = 0;
	u256 maximumExtraDataSize = 1024;
	u256 accountStartNonce = 0;

	std::unordered_map<std::string, std::string> otherParams;
	u256 u256Param(std::string const& _name) const;
	// Ethash specific:
	//u256 minGasLimit;
	//u256 gasLimitBoundDivisor;
	//u256 minimumDifficulty;
	//u256 difficultyBoundDivisor;
	//u256 durationLimit;
};

}
}
