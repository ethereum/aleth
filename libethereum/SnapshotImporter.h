// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Class for importing snapshot from directory on disk
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

    Logger m_logger{createLogger(VerbosityInfo, "snap")};
};

}
}
