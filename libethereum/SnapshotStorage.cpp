// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "SnapshotStorage.h"
#include <libdevcore/CommonIO.h>
#include <libdevcore/SHA3.h>

#include <snappy.h>

namespace fs = boost::filesystem;

namespace dev
{
namespace eth
{

namespace
{

size_t const c_maxChunkUncomressedSize = 10 * 1024 * 1024;


std::string snappyUncompress(std::string const& _compressed)
{
	size_t uncompressedSize = 0;
	if (!snappy::GetUncompressedLength(_compressed.data(), _compressed.size(), &uncompressedSize))
		BOOST_THROW_EXCEPTION(FailedToGetUncompressedLength());

	if (uncompressedSize > c_maxChunkUncomressedSize)
		BOOST_THROW_EXCEPTION(ChunkIsTooBig());

	std::string uncompressed;
	if (!snappy::Uncompress(_compressed.data(), _compressed.size(), &uncompressed))
		BOOST_THROW_EXCEPTION(FailedToUncompressedSnapshotChunk());

	return uncompressed;
}

class SnapshotStorage: public SnapshotStorageFace
{
public:
    explicit SnapshotStorage(fs::path const& _snapshotDir) : m_snapshotDir(_snapshotDir) {}

    bytes readManifest() const override
	{
		bytes const manifestBytes = dev::contents((m_snapshotDir / "MANIFEST").string());
		if (manifestBytes.empty())
			BOOST_THROW_EXCEPTION(FailedToReadSnapshotManifestFile());

		return manifestBytes;
	}

    std::string readCompressedChunk(h256 const& _chunkHash) const override
    {
		std::string const chunkCompressed = dev::contentsString((m_snapshotDir / toHex(_chunkHash)).string());
		if (chunkCompressed.empty())
			BOOST_THROW_EXCEPTION(FailedToReadChunkFile() << errinfo_hash256(_chunkHash));

        return chunkCompressed;
    }

    std::string readChunk(h256 const& _chunkHash) const override
    {
        std::string const chunkCompressed = readCompressedChunk(_chunkHash);

        h256 const chunkHash = sha3(chunkCompressed);
		if (chunkHash != _chunkHash)
			BOOST_THROW_EXCEPTION(ChunkDataCorrupted() << errinfo_hash256(_chunkHash));

		std::string const chunkUncompressed = snappyUncompress(chunkCompressed);
		assert(!chunkUncompressed.empty());

		return chunkUncompressed;
	}

    void copyTo(fs::path const& _path) const override { copyDirectory(m_snapshotDir, _path); }

private:
    fs::path const m_snapshotDir;
};

}

std::unique_ptr<SnapshotStorageFace> createSnapshotStorage(fs::path const& _snapshotDirPath)
{
	return std::unique_ptr<SnapshotStorageFace>(new SnapshotStorage(_snapshotDirPath));
}

fs::path importedSnapshotPath(fs::path const& _dataDir, h256 const& _genesisHash)
{
    return _dataDir / toHex(_genesisHash.ref().cropped(0, 4)) / "snapshot";
}
}
}