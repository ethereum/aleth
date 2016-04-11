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
/** @file EthashProofOfWork.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Determines the PoW algorithm.
 */

#include "EthashProofOfWork.h"
#include "Ethash.h"
using namespace std;
using namespace chrono;
using namespace dev;
using namespace eth;

const unsigned EthashProofOfWork::defaultLocalWorkSize = 64;
const unsigned EthashProofOfWork::defaultGlobalWorkSizeMultiplier = 4096; // * CL_DEFAULT_LOCAL_WORK_SIZE
const unsigned EthashProofOfWork::defaultMSPerBatch = 0;
const EthashProofOfWork::WorkPackage EthashProofOfWork::NullWorkPackage = EthashProofOfWork::WorkPackage();

EthashProofOfWork::WorkPackage::WorkPackage(BlockHeader const& _bh):
	boundary(Ethash::boundary(_bh)),
	headerHash(_bh.hash(WithoutSeal)),
	seedHash(Ethash::seedHash(_bh))
{}
