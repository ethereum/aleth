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

#include "SnapshotImporter.h"
#include "Client.h"

#include <libdevcore/RLP.h>
#include <libdevcore/TrieHash.h>
#include <libethashseal/Ethash.h>

#include <snappy.h>

using namespace dev;
using namespace eth;

namespace
{

struct SnapshotImportLog: public LogChannel
{
	static char const* name() { return "SNAP"; }
	static int const verbosity = 9;
	static const bool debug = false;
};


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

std::string readChunk(boost::filesystem::path const& _snapshotDir, h256 const& _chunkHash)
{
	std::string const chunkCompressed = dev::contentsString((_snapshotDir / toHex(_chunkHash)).string());
	if (chunkCompressed.empty())
		BOOST_THROW_EXCEPTION(FailedToReadChunkFile());

	h256 const chunkHash = sha3(chunkCompressed);
	if (chunkHash != _chunkHash)
		BOOST_THROW_EXCEPTION(ChunkDataCorrupted());

	std::string const chunkUncompressed = snappyUncompress(chunkCompressed);
	assert(!chunkUncompressed.empty());

	return chunkUncompressed;
}

}

void SnapshotImporter::import(std::string const& _snapshotDirPath)
{
	(void)SnapshotImportLog::debug; // override "unused variable" error on macOS

	boost::filesystem::path const snapshotDir(_snapshotDirPath);

	bytes const manifestBytes = dev::contents((snapshotDir / "MANIFEST").string());
	if (manifestBytes.empty())
		BOOST_THROW_EXCEPTION(FailedToReadSnapshotManifestFile());

	RLP manifest(manifestBytes);

	// For Snapshot format see https://github.com/paritytech/parity/wiki/Warp-Sync-Snapshot-Format
	u256 const version = manifest[0].toInt<u256>(RLP::VeryStrict);
	if (version != 2)
		BOOST_THROW_EXCEPTION(UnsupportedSnapshotManifestVersion());
	if (manifest.itemCount() != 6)
		BOOST_THROW_EXCEPTION(InvalidSnapshotManifest());

	u256 const blockNumber = manifest[4].toInt<u256>(RLP::VeryStrict);
	h256 const blockHash = manifest[5].toHash<h256>(RLP::VeryStrict);
	clog(SnapshotImportLog) << "Importing snapshot for block " << blockNumber << " block hash " << blockHash;

	h256s const stateChunkHashes = manifest[1].toVector<h256>(RLP::VeryStrict);
	h256 const stateRoot = manifest[3].toHash<h256>(RLP::VeryStrict);
	importStateChunks(snapshotDir, stateChunkHashes, stateRoot);

	h256s const blockChunkHashes = manifest[2].toVector<h256>(RLP::VeryStrict);
	importBlocks(snapshotDir, blockChunkHashes);
}

void SnapshotImporter::importStateChunks(boost::filesystem::path const& _snapshotDir, h256s const& _stateChunkHashes, h256 const& _stateRoot)
{
	size_t const stateChunkCount = _stateChunkHashes.size();

	StateImporter stateImporter = m_client.createStateImporter();
	std::map<h256, bytes> storageMap;
	h256 addressHash;
	u256 nonce;
	u256 balance;
	h256 codeHash;

	size_t chunksImported = 0;
	size_t accountsImported = 0;

	for (auto const& stateChunkHash: _stateChunkHashes)
	{
		std::string const chunkUncompressed = readChunk(_snapshotDir, stateChunkHash);

		RLP const accounts(chunkUncompressed);
		size_t const accountCount = accounts.itemCount();
		for (size_t accountIndex = 0; accountIndex < accountCount; ++accountIndex)
		{
			RLP const addressAndAccount = accounts[accountIndex];
			if (addressAndAccount.itemCount() != 2)
				BOOST_THROW_EXCEPTION(InvalidStateChunkData());
	
			h256 const addressHashNew = addressAndAccount[0].toHash<h256>(RLP::VeryStrict);
			if (!addressHashNew)
				BOOST_THROW_EXCEPTION(InvalidStateChunkData());
			
			if (addressHash)
			{
				if (addressHashNew != addressHash)
				{
					// previous account was not splitted, so import it
					stateImporter.importAccount(addressHash, nonce, balance, storageMap, codeHash);
					++accountsImported;
					storageMap.clear();
				}
				else
				{
					// splitted account can only be the first in chunk
					if (accountIndex != 0)
						BOOST_THROW_EXCEPTION(InvalidStateChunkData());
				}
			}

			addressHash = addressHashNew;

			RLP const account = addressAndAccount[1];
			if (account.itemCount() != 5)
				BOOST_THROW_EXCEPTION(InvalidStateChunkData());

			nonce = account[0].toInt<u256>(RLP::VeryStrict);
			balance = account[1].toInt<u256>(RLP::VeryStrict);

			RLP const storage = account[4];
			for (auto hashAndValue: storage)
			{
				if (hashAndValue.itemCount() != 2)
					BOOST_THROW_EXCEPTION(InvalidStateChunkData());

				h256 const keyHash = hashAndValue[0].toHash<h256>(RLP::VeryStrict);
				if (!keyHash || storageMap.find(keyHash) != storageMap.end())
					BOOST_THROW_EXCEPTION(InvalidStateChunkData());

				bytes value = hashAndValue[1].toBytes(RLP::VeryStrict);
				if (value.empty())
					BOOST_THROW_EXCEPTION(InvalidStateChunkData());

				storageMap.emplace(keyHash, std::move(value));
			}

			byte const codeFlag = account[2].toInt<byte>(RLP::VeryStrict);
			switch (codeFlag)
			{
			case 0:
				codeHash = EmptySHA3;
				break;
			case 1:
				codeHash = stateImporter.importCode(account[3].toBytesConstRef(RLP::VeryStrict));
				break;
			case 2:
				codeHash = account[3].toHash<h256>(RLP::VeryStrict);
				if (!codeHash || stateImporter.lookupCode(codeHash).empty())
					BOOST_THROW_EXCEPTION(InvalidStateChunkData());
				break;
			default:
				BOOST_THROW_EXCEPTION(InvalidStateChunkData());
			}
		}

		stateImporter.commitStateDatabase();

		++chunksImported;
		clog(SnapshotImportLog) << "Imported chunk " << chunksImported << " (" << accounts.itemCount() << " accounts) Total accounts imported: " << accountsImported;
		clog(SnapshotImportLog) << stateChunkCount - chunksImported << " chunks left to import";
	}

	// last account
	stateImporter.importAccount(addressHash, nonce, balance, storageMap, codeHash);
	stateImporter.commitStateDatabase();
	++accountsImported;

	// check root
	clog(SnapshotImportLog) << "Chunks imported: " << chunksImported;
	clog(SnapshotImportLog) << "Accounts imported: " << accountsImported;
	clog(SnapshotImportLog) << "Reconstructed state root: " << stateImporter.stateRoot();
	clog(SnapshotImportLog) << "Manifest state root:      " << _stateRoot;
	if (stateImporter.stateRoot() != _stateRoot)
		BOOST_THROW_EXCEPTION(StateTrieReconstructionFailed());
}

void SnapshotImporter::importBlocks(boost::filesystem::path const& _snapshotDir, h256s const& _blockChunkHashes)
{
	BlockChainImporter bcImporter = m_client.createBlockChainImporter();

	size_t const blockChunkCount = _blockChunkHashes.size();
	size_t blockChunksImported = 0;
	// chunks are in decreasing order of first block number, so we go backwards to start from the oldest block
	for (auto chunk = _blockChunkHashes.rbegin(); chunk != _blockChunkHashes.rend(); ++chunk)
	{
		std::string const chunkUncompressed = readChunk(_snapshotDir, *chunk);

		RLP blockChunk(chunkUncompressed);
		if (blockChunk.itemCount() < 3)
			BOOST_THROW_EXCEPTION(InvalidBlockChunkData());

		u256 const firstBlockNumber = blockChunk[0].toInt<u256>(RLP::VeryStrict);
		h256 const firstBlockHash = blockChunk[1].toHash<h256>(RLP::VeryStrict);
		u256 const firstBlockDifficulty = blockChunk[2].toInt<u256>(RLP::VeryStrict);
		if (!firstBlockNumber || firstBlockHash || firstBlockDifficulty)
			BOOST_THROW_EXCEPTION(InvalidBlockChunkData());

		clog(SnapshotImportLog) << "chunk first block " << firstBlockNumber << " first block hash " << firstBlockHash << " difficulty " << firstBlockDifficulty;

		size_t const itemCount = blockChunk.itemCount();
		h256 parentHash = firstBlockHash;
		u256 number = firstBlockNumber + 1;
		u256 totalDifficulty = firstBlockDifficulty;
		for (size_t i = 3; i < itemCount; ++i, ++number)
		{
			RLP blockAndReceipts = blockChunk[i];
			if (blockAndReceipts.itemCount() != 2)
				BOOST_THROW_EXCEPTION(InvalidBlockChunkData());

			RLP abridgedBlock = blockAndReceipts[0];

			BlockHeader header;
			header.setParentHash(parentHash);
			header.setAuthor(abridgedBlock[0].toHash<Address>(RLP::VeryStrict));

			h256 const blockStateRoot = abridgedBlock[1].toHash<h256>(RLP::VeryStrict);
			RLP transactions = abridgedBlock[8];
			h256 const txRoot = trieRootOver(transactions.itemCount(), [&](unsigned i) { return rlp(i); }, [&](unsigned i) { return transactions[i].data().toBytes(); });
			RLP uncles = abridgedBlock[9];
			RLP receipts = blockAndReceipts[1];
			std::vector<bytesConstRef> receiptsVector;
			for (auto receipt: receipts)
				receiptsVector.push_back(receipt.data());
			h256 const receiptsRoot = orderedTrieRoot(receiptsVector);
			h256 const unclesHash = sha3(uncles.data());
			header.setRoots(txRoot, receiptsRoot, unclesHash, blockStateRoot);

			header.setLogBloom(abridgedBlock[2].toHash<LogBloom>(RLP::VeryStrict));
			u256 const difficulty = abridgedBlock[3].toInt<u256>(RLP::VeryStrict);
			header.setDifficulty(difficulty);
			header.setNumber(number);
			header.setGasLimit(abridgedBlock[4].toInt<u256>(RLP::VeryStrict));
			header.setGasUsed(abridgedBlock[5].toInt<u256>(RLP::VeryStrict));
			header.setTimestamp(abridgedBlock[6].toInt<u256>(RLP::VeryStrict));
			header.setExtraData(abridgedBlock[7].toBytes(RLP::VeryStrict));

			Ethash::setMixHash(header, abridgedBlock[10].toHash<h256>(RLP::VeryStrict));
			Ethash::setNonce(header, abridgedBlock[11].toHash<Nonce>(RLP::VeryStrict));

			totalDifficulty += difficulty;
			bcImporter.importBlock(header, transactions, uncles, receipts, totalDifficulty);

			parentHash = header.hash();
		}

		clog(SnapshotImportLog) << "Imported chunk " << *chunk << " (" << itemCount - 3 << " blocks)";
		clog(SnapshotImportLog) << blockChunkCount - (++blockChunksImported) << " chunks left to import";

		if (chunk == _blockChunkHashes.rbegin())
		{
			clog(SnapshotImportLog) << "Setting chain start block: " << firstBlockNumber + 1;
			bcImporter.setChainStartBlockNumber(firstBlockNumber + 1);
		}
	}
}
