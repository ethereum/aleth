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
/** @file ChainParams.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <libdevcore/Common.h>
#include <libethcore/Common.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/BlockInfo.h>
#include "Account.h"

namespace dev
{
namespace eth
{

class SealEngineFace;

struct ChainParams: public ChainOperationParams
{
	ChainParams() = default;
	ChainParams(eth::Network _n);
	ChainParams(std::string const& _s);
	ChainParams(eth::Network _n, bytes const& _genesisRLP, AccountMap const& _state);

	SealEngineFace* createSealEngine();

	/// Genesis params.
	h256 parentHash;
	h160 author;
	u256 difficulty;
	u256 gasLimit;
	u256 gasUsed;
	u256 timestamp;
	bytes extraData;
	mutable h256 stateRoot;	///< Only pre-populate if known equivalent to genesisState's root. If they're different Bad Things Will Happen.
	AccountMap genesisState;

	unsigned sealFields = 0;
	bytes sealRLP;

	h256 calculateStateRoot() const;

	/// Genesis block info.
	bytes genesisBlock() const;
};

}
}
