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

#include "SnapshotStorage.h"
#include <libdevcore/CommonIO.h>
#include <libdevcore/SHA3.h>

#include <snappy.h>

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
	explicit SnapshotStorage(boost::filesystem::path const& _snapshotDir): m_snapshotDir(_snapshotDir) {}

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

	void copyTo(boost::filesystem::path const& _path) const override
	{
		copyDirectory(m_snapshotDir, _path);
	}

private:
	boost::filesystem::path const m_snapshotDir;
};

}

std::unique_ptr<SnapshotStorageFace> createSnapshotStorage(boost::filesystem::path const& _snapshotDirPath)
{
	return std::unique_ptr<SnapshotStorageFace>(new SnapshotStorage(_snapshotDirPath));
}

}
}