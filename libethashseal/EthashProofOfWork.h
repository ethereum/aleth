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
/** @file EthashProofOfWork.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 *
 * Determines the PoW algorithm.
 */

#pragma once

#include <libethcore/BlockHeader.h>
#include <libdevcore/Guards.h>

namespace dev
{
namespace eth
{

/// Proof of work definition for Ethash.
struct EthashProofOfWork
{
	struct Solution
	{
		Nonce nonce;
		h256 mixHash;
	};

	struct Result
	{
		h256 value;
		h256 mixHash;
	};

	struct WorkPackage
	{
		WorkPackage() {}
		WorkPackage(BlockHeader const& _bh);
		WorkPackage(WorkPackage const& _other);
		WorkPackage& operator=(WorkPackage const& _other);

		void reset() { Guard l(m_headerHashLock); m_headerHash = h256(); }
		operator bool() const { Guard l(m_headerHashLock); return m_headerHash != h256(); }
		h256 headerHash() const { Guard l(m_headerHashLock); return m_headerHash; }

		h256 boundary;
		h256 seedHash;

	private:
		h256 m_headerHash;	///< When h256() means "pause until notified a new work package is available".
		mutable Mutex m_headerHashLock;
	};

	static const WorkPackage NullWorkPackage;

	/// Default value of the local work size. Also known as workgroup size.
	static const unsigned defaultLocalWorkSize;
	/// Default value of the global work size as a multiplier of the local work size
	static const unsigned defaultGlobalWorkSizeMultiplier;
	/// Default value of the milliseconds per global work size (per batch)
	static const unsigned defaultMSPerBatch;
};

}
}
