// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>

#include <boost/filesystem.hpp>

#include <memory>

namespace dev
{

namespace eth
{

DEV_SIMPLE_EXCEPTION(FailedToReadSnapshotManifestFile);
DEV_SIMPLE_EXCEPTION(FailedToReadChunkFile);
DEV_SIMPLE_EXCEPTION(ChunkIsTooBig);
DEV_SIMPLE_EXCEPTION(ChunkDataCorrupted);
DEV_SIMPLE_EXCEPTION(FailedToGetUncompressedLength);
DEV_SIMPLE_EXCEPTION(FailedToUncompressedSnapshotChunk);

class SnapshotStorageFace
{
public:
	virtual ~SnapshotStorageFace() = default;

	virtual bytes readManifest() const = 0;

    virtual std::string readCompressedChunk(h256 const& _chunkHash) const = 0;

    virtual std::string readChunk(h256 const& _chunkHash) const = 0;

    virtual void copyTo(boost::filesystem::path const& _path) const = 0;
};


std::unique_ptr<SnapshotStorageFace> createSnapshotStorage(
    boost::filesystem::path const& _snapshotDirPath);

boost::filesystem::path importedSnapshotPath(
    boost::filesystem::path const& _dataDir, h256 const& _genesisHash);
}
}
