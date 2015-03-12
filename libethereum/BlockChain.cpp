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
/** @file BlockChain.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <leveldb/db.h>

#include "BlockChain.h"

#include <boost/filesystem.hpp>
#include <test/JsonSpiritHeaders.h>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/StructuredLogger.h>
#include <libdevcrypto/FileSystem.h>
#include <libethcore/Exceptions.h>
#include <libethcore/ProofOfWork.h>
#include <libethcore/BlockInfo.h>
#include <liblll/Compiler.h>
#include "GenesisInfo.h"
#include "State.h"
#include "Defaults.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace js = json_spirit;

#define ETH_CATCH 1

std::ostream& dev::eth::operator<<(std::ostream& _out, BlockChain const& _bc)
{
	string cmp = toBigEndianString(_bc.currentHash());
	auto it = _bc.m_blocksDB->NewIterator(_bc.m_readOptions);
	for (it->SeekToFirst(); it->Valid(); it->Next())
		if (it->key().ToString() != "best")
		{
			try {
				BlockInfo d(bytesConstRef(it->value()));
				_out << toHex(it->key().ToString()) << ":   " << d.number << " @ " << d.parentHash << (cmp == it->key().ToString() ? "  BEST" : "") << std::endl;
			}
			catch (...) {
				cwarn << "Invalid DB entry:" << toHex(it->key().ToString()) << " -> " << toHex(bytesConstRef(it->value()));
			}
		}
	delete it;
	return _out;
}

ldb::Slice dev::eth::toSlice(h256 _h, unsigned _sub)
{
#if ALL_COMPILERS_ARE_CPP11_COMPLIANT
	static thread_local h256 h = _h ^ h256(u256(_sub));
	return ldb::Slice((char const*)&h, 32);
#else
	static boost::thread_specific_ptr<h256> t_h;
	if (!t_h.get())
		t_h.reset(new h256);
	*t_h = _h ^ h256(u256(_sub));
	return ldb::Slice((char const*)t_h.get(), 32);
#endif
}

#if ETH_DEBUG
static const chrono::system_clock::duration c_collectionDuration = chrono::seconds(15);
static const unsigned c_collectionQueueSize = 2;
static const unsigned c_maxCacheSize = 1024 * 1024 * 1;
static const unsigned c_minCacheSize = 1;
#else

/// Duration between flushes.
static const chrono::system_clock::duration c_collectionDuration = chrono::seconds(60);

/// Length of death row (total time in cache is multiple of this and collection duration).
static const unsigned c_collectionQueueSize = 20;

/// Max size, above which we start forcing cache reduction.
static const unsigned c_maxCacheSize = 1024 * 1024 * 64;

/// Min size, below which we don't bother flushing it.
static const unsigned c_minCacheSize = 1024 * 1024 * 32;

#endif

BlockChain::BlockChain(bytes const& _genesisBlock, std::string _path, bool _killExisting)
{
	// initialise deathrow.
	m_cacheUsage.resize(c_collectionQueueSize);
	m_lastCollection = chrono::system_clock::now();

	// Initialise with the genesis as the last block on the longest chain.
	m_genesisBlock = _genesisBlock;
	m_genesisHash = sha3(RLP(m_genesisBlock)[0].data());

	open(_path, _killExisting);
}

BlockChain::~BlockChain()
{
	close();
}

void BlockChain::open(std::string _path, bool _killExisting)
{
	if (_path.empty())
		_path = Defaults::get()->m_dbPath;
	boost::filesystem::create_directories(_path);
	if (_killExisting)
	{
		boost::filesystem::remove_all(_path + "/blocks");
		boost::filesystem::remove_all(_path + "/details");
	}

	ldb::Options o;
	o.create_if_missing = true;
	ldb::DB::Open(o, _path + "/blocks", &m_blocksDB);
	ldb::DB::Open(o, _path + "/details", &m_extrasDB);
	if (!m_blocksDB)
		BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
	if (!m_extrasDB)
		BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());

	if (!details(m_genesisHash))
	{
		// Insert details of genesis block.
		m_details[m_genesisHash] = BlockDetails(0, c_genesisDifficulty, h256(), {});
		auto r = m_details[m_genesisHash].rlp();
		m_extrasDB->Put(m_writeOptions, ldb::Slice((char const*)&m_genesisHash, 32), (ldb::Slice)dev::ref(r));
	}

	checkConsistency();

	// TODO: Implement ability to rebuild details map from DB.
	std::string l;
	m_extrasDB->Get(m_readOptions, ldb::Slice("best"), &l);

	m_lastBlockHash = l.empty() ? m_genesisHash : *(h256*)l.data();

	cnote << "Opened blockchain DB. Latest: " << currentHash();
}

void BlockChain::close()
{
	cnote << "Closing blockchain DB";
	delete m_extrasDB;
	delete m_blocksDB;
	m_lastBlockHash = m_genesisHash;
	m_details.clear();
	m_blocks.clear();
}

template <class T, class V>
bool contains(T const& _t, V const& _v)
{
	for (auto const& i: _t)
		if (i == _v)
			return true;
	return false;
}

inline string toString(h256s const& _bs)
{
	ostringstream out;
	out << "[ ";
	for (auto i: _bs)
		out << i.abridged() << ", ";
	out << "]";
	return out.str();
}

h256s BlockChain::sync(BlockQueue& _bq, OverlayDB const& _stateDB, unsigned _max)
{
	_bq.tick(*this);

	vector<bytes> blocks;
	_bq.drain(blocks);

	h256s ret;
	for (auto const& block: blocks)
	{
		try
		{
			for (auto h: import(block, _stateDB))
				if (!_max--)
					break;
				else
					ret.push_back(h);
		}
		catch (UnknownParent)
		{
			cwarn << "Unknown parent of block!!!" << BlockInfo::headerHash(block).abridged() << boost::current_exception_diagnostic_information();
			_bq.import(&block, *this);
		}
		catch (Exception const& _e)
		{
			cwarn << "Unexpected exception!" << diagnostic_information(_e);
			_bq.import(&block, *this);
		}
		catch (...)
		{}
	}
	_bq.doneDrain();
	return ret;
}

h256s BlockChain::attemptImport(bytes const& _block, OverlayDB const& _stateDB) noexcept
{
	try
	{
		return import(_block, _stateDB);
	}
	catch (...)
	{
		cwarn << "Unexpected exception! Could not import block!" << boost::current_exception_diagnostic_information();
		return h256s();
	}
}

h256s BlockChain::import(bytes const& _block, OverlayDB const& _db)
{
	// VERIFY: populates from the block and checks the block is internally coherent.
	BlockInfo bi;

#if ETH_CATCH
	try
#endif
	{
		RLP blockRLP(_block);

		if (!blockRLP.isList())
			BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("block header needs to be a list") << BadFieldError(0, blockRLP.data().toString()));

		bi.populate(&_block);
		bi.verifyInternals(&_block);
	}
#if ETH_CATCH
	catch (Exception const& _e)
	{
		clog(BlockChainNote) << "   Malformed block: " << diagnostic_information(_e);
		_e << errinfo_comment("Malformed block ");
		throw;
	}
#endif
	auto newHash = BlockInfo::headerHash(_block);

	// Check block doesn't already exist first!
	if (isKnown(newHash))
	{
		clog(BlockChainNote) << newHash << ": Not new.";
		BOOST_THROW_EXCEPTION(AlreadyHaveBlock());
	}

	// Work out its number as the parent's number + 1
	if (!isKnown(bi.parentHash))
	{
		clog(BlockChainNote) << newHash << ": Unknown parent " << bi.parentHash;
		// We don't know the parent (yet) - discard for now. It'll get resent to us if we find out about its ancestry later on.
		BOOST_THROW_EXCEPTION(UnknownParent());
	}

	auto pd = details(bi.parentHash);
	if (!pd)
	{
		auto pdata = pd.rlp();
		cwarn << "Odd: details is returning false despite block known:" << RLP(pdata);
		auto parentBlock = block(bi.parentHash);
		cwarn << "Block:" << RLP(parentBlock);
	}

	// Check it's not crazy
	if (bi.timestamp > (u256)time(0))
	{
		clog(BlockChainNote) << newHash << ": Future time " << bi.timestamp << " (now at " << time(0) << ")";
		// Block has a timestamp in the future. This is no good.
		BOOST_THROW_EXCEPTION(FutureTime());
	}

	clog(BlockChainNote) << "Attempting import of " << newHash.abridged() << "...";

	u256 td;
#if ETH_CATCH
	try
#endif
	{
		// Check transactions are valid and that they result in a state equivalent to our state_root.
		// Get total difficulty increase and update state, checking it.
		State s(bi.coinbaseAddress, _db);
		auto tdIncrease = s.enactOn(&_block, bi, *this);
		BlockLogBlooms blb;
		BlockReceipts br;
		for (unsigned i = 0; i < s.pending().size(); ++i)
		{
			blb.blooms.push_back(s.receipt(i).bloom());
			br.receipts.push_back(s.receipt(i));
		}
		s.cleanup(true);
		td = pd.totalDifficulty + tdIncrease;

#if ETH_PARANOIA
		checkConsistency();
#endif
		// All ok - insert into DB
		{
			WriteGuard l(x_details);
			m_details[newHash] = BlockDetails((unsigned)pd.number + 1, td, bi.parentHash, {});
			m_details[bi.parentHash].children.push_back(newHash);
		}
		{
			WriteGuard l(x_blockHashes);
			m_blockHashes[h256(bi.number)].value = newHash;
		}
		{
			WriteGuard l(x_blocksBlooms);
			LogBloom blockBloom = bi.logBloom;
			blockBloom.shiftBloom<3, 32>(sha3(bi.coinbaseAddress.ref()));
			unsigned index = (unsigned)bi.number;
			for (unsigned level = 0; level < c_bloomIndexLevels; level++, index /= c_bloomIndexSize)
			{
				unsigned i = index / c_bloomIndexSize % c_bloomIndexSize;
				unsigned o = index % c_bloomIndexSize;
				m_blocksBlooms[chunkId(level, i)].blooms[o] |= blockBloom;
			}
		}
		// Collate transaction hashes and remember who they were.
		h256s tas;
		{
			RLP blockRLP(_block);
			TransactionAddress ta;
			ta.blockHash = newHash;
			WriteGuard l(x_transactionAddresses);
			for (ta.index = 0; ta.index < blockRLP[1].itemCount(); ++ta.index)
			{
				tas.push_back(sha3(blockRLP[1][ta.index].data()));
				m_transactionAddresses[tas.back()] = ta;
			}
		}
		{
			WriteGuard l(x_logBlooms);
			m_logBlooms[newHash] = blb;
		}
		{
			WriteGuard l(x_receipts);
			m_receipts[newHash] = br;
		}

		{
			ReadGuard l1(x_blocksBlooms);
			ReadGuard l2(x_details);
			ReadGuard l3(x_blockHashes);
			ReadGuard l4(x_receipts);
			ReadGuard l5(x_logBlooms);
			ReadGuard l6(x_transactionAddresses);
			m_blocksDB->Put(m_writeOptions, toSlice(newHash), (ldb::Slice)ref(_block));
			m_extrasDB->Put(m_writeOptions, toSlice(newHash, ExtraDetails), (ldb::Slice)dev::ref(m_details[newHash].rlp()));
			m_extrasDB->Put(m_writeOptions, toSlice(bi.parentHash, ExtraDetails), (ldb::Slice)dev::ref(m_details[bi.parentHash].rlp()));
			m_extrasDB->Put(m_writeOptions, toSlice(h256(bi.number), ExtraBlockHash), (ldb::Slice)dev::ref(m_blockHashes[h256(bi.number)].rlp()));
			for (auto const& h: tas)
				m_extrasDB->Put(m_writeOptions, toSlice(h, ExtraTransactionAddress), (ldb::Slice)dev::ref(m_transactionAddresses[h].rlp()));
			m_extrasDB->Put(m_writeOptions, toSlice(newHash, ExtraLogBlooms), (ldb::Slice)dev::ref(m_logBlooms[newHash].rlp()));
			m_extrasDB->Put(m_writeOptions, toSlice(newHash, ExtraReceipts), (ldb::Slice)dev::ref(m_receipts[newHash].rlp()));
			m_extrasDB->Put(m_writeOptions, toSlice(newHash, ExtraBlocksBlooms), (ldb::Slice)dev::ref(m_blocksBlooms[newHash].rlp()));
		}

#if ETH_PARANOIA
		checkConsistency();
#endif
	}
#if ETH_CATCH
	catch (Exception const& _e)
	{
		clog(BlockChainNote) << "   Malformed block: " << diagnostic_information(_e);
		_e << errinfo_comment("Malformed block ");
		throw;
	}
#endif

	StructuredLogger::chainReceivedNewBlock(
		bi.headerHash(WithoutNonce).abridged(),
		bi.nonce.abridged(),
		currentHash().abridged(),
		"", // TODO: remote id ??
		bi.parentHash.abridged()
	);
	//	cnote << "Parent " << bi.parentHash << " has " << details(bi.parentHash).children.size() << " children.";

	h256s ret;
	// This might be the new best block...
	h256 last = currentHash();
	if (td > details(last).totalDifficulty)
	{
		ret = treeRoute(last, newHash);
		{
			WriteGuard l(x_lastBlockHash);
			m_lastBlockHash = newHash;
		}
		m_extrasDB->Put(m_writeOptions, ldb::Slice("best"), ldb::Slice((char const*)&newHash, 32));
		clog(BlockChainNote) << "   Imported and best" << td << ". Has" << (details(bi.parentHash).children.size() - 1) << "siblings. Route:" << toString(ret);
		StructuredLogger::chainNewHead(
			bi.headerHash(WithoutNonce).abridged(),
			bi.nonce.abridged(),
			currentHash().abridged(),
			bi.parentHash.abridged()
		);
	}
	else
	{
		clog(BlockChainNote) << "   Imported but not best (oTD:" << details(last).totalDifficulty << " > TD:" << td << ")";
	}
	return ret;
}

h256s BlockChain::treeRoute(h256 _from, h256 _to, h256* o_common, bool _pre, bool _post) const
{
	//	cdebug << "treeRoute" << _from.abridged() << "..." << _to.abridged();
	if (!_from || !_to)
	{
		return h256s();
	}
	h256s ret;
	h256s back;
	unsigned fn = details(_from).number;
	unsigned tn = details(_to).number;
	//	cdebug << "treeRoute" << fn << "..." << tn;
	while (fn > tn)
	{
		if (_pre)
			ret.push_back(_from);
		_from = details(_from).parent;
		fn--;
		//		cdebug << "from:" << fn << _from.abridged();
	}
	while (fn < tn)
	{
		if (_post)
			back.push_back(_to);
		_to = details(_to).parent;
		tn--;
		//		cdebug << "to:" << tn << _to.abridged();
	}
	while (_from != _to)
	{
		assert(_from);
		assert(_to);
		_from = details(_from).parent;
		_to = details(_to).parent;
		if (_pre)
			ret.push_back(_from);
		if (_post)
			back.push_back(_to);
		fn--;
		tn--;
		//		cdebug << "from:" << fn << _from.abridged() << "; to:" << tn << _to.abridged();
	}
	if (o_common)
		*o_common = _from;
	ret.reserve(ret.size() + back.size());
	for (auto it = back.cbegin(); it != back.cend(); ++it)
		ret.push_back(*it);
	return ret;
}

void BlockChain::noteUsed(h256 const& _h, unsigned _extra) const
{
	auto id = CacheID(_h, _extra);
	Guard l(x_cacheUsage);
	m_cacheUsage[0].insert(id);
	if (m_cacheUsage[1].count(id))
		m_cacheUsage[1].erase(id);
	else
		m_inUse.insert(id);
}

template <class T> static unsigned getHashSize(map<h256, T> const& _map)
{
	unsigned ret = 0;
	for (auto const& i: _map)
		ret += i.second.size + 64;
	return ret;
}

void BlockChain::updateStats() const
{
	{
		ReadGuard l(x_blocks);
		m_lastStats.memBlocks = 0;
		for (auto const& i: m_blocks)
			m_lastStats.memBlocks += i.second.size() + 64;
	}
	{
		ReadGuard l(x_details);
		m_lastStats.memDetails = getHashSize(m_details);
	}
	{
		ReadGuard l1(x_logBlooms);
		ReadGuard l2(x_blocksBlooms);
		m_lastStats.memLogBlooms = getHashSize(m_logBlooms) + getHashSize(m_blocksBlooms);
	}
	{
		ReadGuard l(x_receipts);
		m_lastStats.memReceipts = getHashSize(m_receipts);
	}
	{
		ReadGuard l(x_blockHashes);
		m_lastStats.memBlockHashes = getHashSize(m_blockHashes);
	}
	{
		ReadGuard l(x_transactionAddresses);
		m_lastStats.memTransactionAddresses = getHashSize(m_transactionAddresses);
	}
}

void BlockChain::garbageCollect(bool _force)
{
	updateStats();

	if (!_force && chrono::system_clock::now() < m_lastCollection + c_collectionDuration && m_lastStats.memTotal() < c_maxCacheSize)
		return;
	if (m_lastStats.memTotal() < c_minCacheSize)
		return;

	m_lastCollection = chrono::system_clock::now();

	Guard l(x_cacheUsage);
	WriteGuard l1(x_blocks);
	WriteGuard l2(x_details);
	WriteGuard l3(x_blockHashes);
	WriteGuard l4(x_receipts);
	WriteGuard l5(x_logBlooms);
	WriteGuard l6(x_transactionAddresses);
	WriteGuard l7(x_blocksBlooms);
	for (CacheID const& id: m_cacheUsage.back())
	{
		m_inUse.erase(id);
		// kill i from cache.
		switch (id.second)
		{
		case (unsigned)-1:
			m_blocks.erase(id.first);
			break;
		case ExtraDetails:
			m_details.erase(id.first);
			break;
		case ExtraBlockHash:
			m_blockHashes.erase(id.first);
			break;
		case ExtraReceipts:
			m_receipts.erase(id.first);
			break;
		case ExtraLogBlooms:
			m_logBlooms.erase(id.first);
			break;
		case ExtraTransactionAddress:
			m_transactionAddresses.erase(id.first);
			break;
		case ExtraBlocksBlooms:
			m_blocksBlooms.erase(id.first);
			break;
		}
	}
	m_cacheUsage.pop_back();
	m_cacheUsage.push_front(std::set<CacheID>{});
}

void BlockChain::checkConsistency()
{
	{
		WriteGuard l(x_details);
		m_details.clear();
	}
	ldb::Iterator* it = m_blocksDB->NewIterator(m_readOptions);
	for (it->SeekToFirst(); it->Valid(); it->Next())
		if (it->key().size() == 32)
		{
			h256 h((byte const*)it->key().data(), h256::ConstructFromPointer);
			auto dh = details(h);
			auto p = dh.parent;
			if (p != h256() && p != m_genesisHash)	// TODO: for some reason the genesis details with the children get squished. not sure why.
			{
				auto dp = details(p);
				if (asserts(contains(dp.children, h)))
				{
					cnote << "Apparently the database is corrupt. Not much we can do at this stage...";
				}
				if (assertsEqual(dp.number, dh.number - 1))
				{
					cnote << "Apparently the database is corrupt. Not much we can do at this stage...";
				}
			}
		}
	delete it;
}

static inline unsigned upow(unsigned a, unsigned b) { while (b-- > 0) a *= a; return a; }
static inline unsigned ceilDiv(unsigned n, unsigned d) { return n / (n + d - 1); }
static inline unsigned floorDivPow(unsigned n, unsigned a, unsigned b) { return n / upow(a, b); }
static inline unsigned ceilDivPow(unsigned n, unsigned a, unsigned b) { return ceilDiv(n, upow(a, b)); }

// Level 1
// [xxx.            ]

// Level 0
// [.x............F.]
// [........x.......]
// [T.............x.]
// [............    ]

// F = 14. T = 32

vector<unsigned> BlockChain::withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest) const
{
	vector<unsigned> ret;

	// start from the top-level
	unsigned u = upow(c_bloomIndexSize, c_bloomIndexLevels);

	// run through each of the top-level blockbloom blocks
	for (unsigned index = _earliest / u; index <= ceilDiv(_latest, u); ++index)				// 0
		ret += withBlockBloom(_b, _earliest, _latest, c_bloomIndexLevels - 1, index);

	return ret;
}

vector<unsigned> BlockChain::withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest, unsigned _level, unsigned _index) const
{
	// 14, 32, 1, 0
		// 14, 32, 0, 0
		// 14, 32, 0, 1
		// 14, 32, 0, 2

	vector<unsigned> ret;

	unsigned uCourse = upow(c_bloomIndexSize, _level + 1);
	// 256
		// 16
	unsigned uFine = upow(c_bloomIndexSize, _level);
	// 16
		// 1

	unsigned obegin = _index == _earliest / uCourse ? _earliest / uFine % c_bloomIndexSize : 0;
	// 0
		// 14
		// 0
		// 0
	unsigned oend = _index == _latest / uCourse ? (_latest / uFine) % c_bloomIndexSize + 1 : c_bloomIndexSize;
	// 3
		// 16
		// 16
		// 1

	BlocksBlooms bb = blocksBlooms(_level, _index);
	for (unsigned o = obegin; o < oend; ++o)
		if (bb.blooms[o].contains(_b))
		{
			// This level has something like what we want.
			if (_level > 0)
				ret += withBlockBloom(_b, _earliest, _latest, _level - 1, o + _index * c_bloomIndexSize);
			else
				ret.push_back(o + _index * c_bloomIndexSize);
		}
	return ret;
}

h256Set BlockChain::allUnclesFrom(h256 _parent) const
{
	// Get all uncles cited given a parent (i.e. featured as uncles/main in parent, parent + 1, ... parent + 5).
	h256Set ret;
	h256 p = _parent;
	for (unsigned i = 0; i < 6 && p != m_genesisHash; ++i, p = details(p).parent)
	{
		ret.insert(p);		// TODO: check: should this be details(p).parent?
		auto b = block(p);
		for (auto i: RLP(b)[2])
			ret.insert(sha3(i.data()));
	}
	return ret;
}

bool BlockChain::isKnown(h256 _hash) const
{
	if (_hash == m_genesisHash)
		return true;
	{
		ReadGuard l(x_blocks);
		if (m_blocks.count(_hash))
			return true;
	}
	string d;
	m_blocksDB->Get(m_readOptions, ldb::Slice((char const*)&_hash, 32), &d);
	return !!d.size();
}

bytes BlockChain::block(h256 _hash) const
{
	if (_hash == m_genesisHash)
		return m_genesisBlock;

	{
		ReadGuard l(x_blocks);
		auto it = m_blocks.find(_hash);
		if (it != m_blocks.end())
			return it->second;
	}

	string d;
	m_blocksDB->Get(m_readOptions, ldb::Slice((char const*)&_hash, 32), &d);

	if (!d.size())
	{
		cwarn << "Couldn't find requested block:" << _hash.abridged();
		return bytes();
	}

	WriteGuard l(x_blocks);
	m_blocks[_hash].resize(d.size());
	memcpy(m_blocks[_hash].data(), d.data(), d.size());

	noteUsed(_hash);

	return m_blocks[_hash];
}
