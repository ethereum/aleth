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

#include <boost/filesystem.hpp>
#include <snappy.h>

using namespace dev;
using namespace eth;

namespace
{

std::string snappyUncompress(std::string const& _compressed)
{
	size_t uncompressedSize = 0;
	if (!snappy::GetUncompressedLength(_compressed.data(), _compressed.size(), &uncompressedSize))
		BOOST_THROW_EXCEPTION(FailedToGetUncompressedLength());

	std::vector<char> uncompressed(uncompressedSize);
	if (!snappy::RawUncompress(_compressed.data(), _compressed.size(), uncompressed.data()))
		BOOST_THROW_EXCEPTION(FailedToUncompressedSnapshotChunk());

	return std::string(uncompressed.begin(), uncompressed.end());
}

class SnapshotStorage: public SnapshotStorageFace
{
public:
	explicit SnapshotStorage(const std::string& _snapshotDir): m_snapshotDir(_snapshotDir) {}

	bytes readManifest() const override
	{
		bytes const manifestBytes = dev::contents((m_snapshotDir / "MANIFEST").string());
		if (manifestBytes.empty())
			BOOST_THROW_EXCEPTION(FailedToReadSnapshotManifestFile());

		return manifestBytes;
	}

	std::string readChunk(h256 const& _chunkHash) const override
	{
		std::string const chunkCompressed = dev::contentsString((m_snapshotDir / toHex(_chunkHash)).string());
		if (chunkCompressed.empty())
			BOOST_THROW_EXCEPTION(FailedToReadChunkFile());

		h256 const chunkHash = sha3(chunkCompressed);
		if (chunkHash != _chunkHash)
			BOOST_THROW_EXCEPTION(ChunkDataCorrupted());

		std::string const chunkUncompressed = snappyUncompress(chunkCompressed);
		assert(!chunkUncompressed.empty());

		return chunkUncompressed;
	}

private:
	boost::filesystem::path const m_snapshotDir;
};

}

std::unique_ptr<SnapshotStorageFace> dev::eth::createSnapshotStorage(std::string const& _snapshotDirPath)
{
	return std::unique_ptr<SnapshotStorageFace>(new SnapshotStorage(_snapshotDirPath));
}
