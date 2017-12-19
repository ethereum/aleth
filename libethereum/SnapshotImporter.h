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
/** @file
 *  Class for importing snapshot from directory on disk
 */

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Log.h>

namespace dev
{

namespace eth
{

class Client;
class BlockChainImporterFace;
class SnapshotStorageFace;
class StateImporterFace;

class SnapshotImporter
{
public:
	SnapshotImporter(StateImporterFace& _stateImporter, BlockChainImporterFace& _bcImporter): m_stateImporter(_stateImporter), m_blockChainImporter(_bcImporter) {}

	void import(SnapshotStorageFace const& _snapshotStorage, h256 const& _genesisHash);

private:
	void importStateChunks(SnapshotStorageFace const& _snapshotStorage, h256s const& _stateChunkHashes, h256 const& _stateRoot);
	void importBlockChunks(SnapshotStorageFace const& _snapshotStorage, h256s const& _blockChunkHashes);

	StateImporterFace& m_stateImporter;
	BlockChainImporterFace& m_blockChainImporter;
};

}
}
