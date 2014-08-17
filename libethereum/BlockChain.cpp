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

#include "BlockChain.h"

#include <atomic>
#include <boost/filesystem.hpp>
#include <libethential/Common.h>
#include <libethential/RLP.h>
#include <libethcore/FileSystem.h>
#include <libethcore/Exceptions.h>
#include <libethcore/Dagger.h>
#include <libethcore/BlockInfo.h>
#include "State.h"
#include "Defaults.h"
using namespace std;
using namespace eth;

#define ETH_CATCH 1

std::ostream& eth::operator<<(std::ostream& _out, BlockChain const& _bc)
{
	string cmp = toBigEndianString(_bc.currentHash());
	auto it = _bc.m_extrasDB->NewIterator(_bc.m_readOptions);
	for (it->SeekToFirst(); it->Valid(); it->Next())
		if (it->key().ToString() != "best")
		{
			BlockDetails d(RLP(it->value().ToString()));
			_out << toHex(it->key().ToString()) << ":   " << d.number << " @ " << d.parent << (cmp == it->key().ToString() ? "  BEST" : "") << std::endl;
		}
	delete it;
	return _out;
}

std::map<Address, AddressState> const& eth::genesisState()
{
	static std::map<Address, AddressState> s_ret;
	if (s_ret.empty())
		// Initialise.
		for (auto i: vector<string>({
            "51ba59315b3a95761d0863b05ccc7a7f54703d99",
			"e6716f9544a56c530d868e4bfbacb172315bdead",
			"b9c015918bdaba24b4ff057a92a3873d6eb201be",
			"1a26338f0d905e295fccb71fa9ea849ffa12aaf4",
			"2ef47100e0787b915105fd5e3f4ff6752079d5cb",
			"cd2a3d9f938e13cd947ec05abc7fe734df8dd826",
			"6c386a4b26f73c802f34673f7248bb118f97424a",
			"e4157b34ea9615cfbde6b4fda419828124b70c78"
		}))
			s_ret[Address(fromHex(i))] = AddressState(0, u256(1) << 200, h256(), EmptySHA3);
	return s_ret;
}

BlockInfo* BlockChain::s_genesis = nullptr;
boost::shared_mutex BlockChain::x_genesis;

ldb::Slice eth::toSlice(h256 _h, unsigned _sub)
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

bytes BlockChain::createGenesisBlock()
{
	RLPStream block(3);
	auto sha3EmptyList = sha3(RLPEmptyList);

	h256 stateRoot;
	{
		MemoryDB db;
		TrieDB<Address, MemoryDB> state(&db);
		state.init();
		eth::commit(genesisState(), db, state);
		stateRoot = state.root();
	}

	block.appendList(13) << h256() << sha3EmptyList << h160();
	block.append(stateRoot, false, true) << bytes() << c_genesisDifficulty << 0 << 0 << 1000000 << 0 << (uint)0 << string() << sha3(bytes(1, 42));
	block.appendRaw(RLPEmptyList);
	block.appendRaw(RLPEmptyList);
	return block.out();
}

BlockChain::BlockChain(std::string _path, bool _killExisting):
	m_stop(true)
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
	auto s = ldb::DB::Open(o, _path + "/blocks", &m_db);
	assert(m_db);
	s = ldb::DB::Open(o, _path + "/details", &m_extrasDB);
	assert(m_extrasDB);

	// Initialise with the genesis as the last block on the longest chain.
	m_genesisHash = BlockChain::genesis().hash;
	m_genesisBlock = BlockChain::createGenesisBlock();

	if (!details(m_genesisHash))
	{
		// Insert details of genesis block.
		m_details[m_genesisHash] = BlockDetails(0, c_genesisDifficulty, h256(), {}, h256());
		auto r = m_details[m_genesisHash].rlp();
		m_extrasDB->Put(m_writeOptions, ldb::Slice((char const*)&m_genesisHash, 32), (ldb::Slice)eth::ref(r));
	}

	checkConsistency();

	// TODO: Implement ability to rebuild details map from DB.
	std::string l;
	m_extrasDB->Get(m_readOptions, ldb::Slice("best"), &l);

	m_lastBlockHash = l.empty() ? m_genesisHash : *(h256*)l.data();

	cnote << "Opened blockchain DB. Latest: " << currentHash();
}

BlockChain::~BlockChain()
{
	cnote << "Closing blockchain DB";
	delete m_extrasDB;
	delete m_db;
}

template <class T, class V>
bool contains(T const& _t, V const& _v)
{
	for (auto const& i: _t)
		if (i == _v)
			return true;
	return false;
}

void BlockChain::run(BlockQueue& _bq, OverlayDB const& _stateDB, std::function<void(h256s _newBlocks, OverlayDB& _stateDB)> _cb)
{
	// always run once to update working state, regardless of _bq.items()
	// useful for mined blocks and new transactions
	lock_guard<std::mutex> l(x_run);
	m_workingStateDB = _stateDB;
	h256s blocks = sync(_bq, m_workingStateDB, 100);
	_cb(blocks, m_workingStateDB);
	
	const char* c_threadName = "chain";
	if (!m_run)
		m_run.reset(new thread([&, c_threadName, _cb]()
		{
			setThreadName(c_threadName);
			m_stop.store(false, std::memory_order_release);
			while (!m_stop.load(std::memory_order_acquire))
			{
				std::lock_guard<std::recursive_mutex> l(m_sync);
				if (_bq.items().first > 0 || _bq.items().second > 0)
				{
					h256s blocks = sync(_bq, m_workingStateDB, 100);
					_cb(blocks, m_workingStateDB);
				} else
					this_thread::sleep_for(chrono::milliseconds(250));
			}
		}));
}

bool BlockChain::running()
{
	std::lock_guard<std::mutex> l(x_run);
	return m_stop ? false : !!m_run;
}

void BlockChain::stop()
{
	m_stop.store(true, std::memory_order_release);
	lock_guard<std::mutex> l(x_run);
	if (m_run)
		m_run->join();
	m_run = nullptr;
}

h256s BlockChain::sync(BlockQueue& _bq, OverlayDB const& _stateDB, unsigned _max)
{
	vector<bytes> blocks;
	std::lock_guard<std::recursive_mutex> l(m_sync);
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
			cwarn << "Unknown parent of block!!!" << eth::sha3(block).abridged();
			_bq.import(&block, *this);
		}
		catch (...){}
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
		bi.populate(&_block);
		bi.verifyInternals(&_block);
	}
#if ETH_CATCH
	catch (Exception const& _e)
	{
		clog(BlockChainNote) << "   Malformed block (" << _e.description() << ").";
		throw;
	}
#endif
	auto newHash = eth::sha3(_block);

	// Check block doesn't already exist first!
	if (details(newHash))
	{
		clog(BlockChainNote) << newHash << ": Not new.";
		throw AlreadyHaveBlock();
	}

	// Work out its number as the parent's number + 1
	auto pd = details(bi.parentHash);
	if (!pd)
	{
		clog(BlockChainNote) << newHash << ": Unknown parent " << bi.parentHash;
		// We don't know the parent (yet) - discard for now. It'll get resent to us if we find out about its ancestry later on.
		throw UnknownParent();
	}

	// Check it's not crazy
	if (bi.timestamp > (u256)time(0))
	{
		clog(BlockChainNote) << newHash << ": Future time " << bi.timestamp << " (now at " << time(0) << ")";
		// Block has a timestamp in the future. This is no good.
		throw FutureTime();
	}

	clog(BlockChainNote) << "Attempting import of " << newHash << "...";

	u256 td;
#if ETH_CATCH
	try
#endif
	{
		// Check transactions are valid and that they result in a state equivalent to our state_root.
		// Get total difficulty increase and update state, checking it.
		State s(bi.coinbaseAddress, _db);
		auto tdIncrease = s.enactOn(&_block, bi, *this);
		auto b = s.bloom();
		BlockBlooms bb;
		BlockTraces bt;
		for (unsigned i = 0; i < s.pending().size(); ++i)
		{
			bt.traces.push_back(s.changesFromPending(i));
			bb.blooms.push_back(s.changesFromPending(i).bloom());
		}
		s.cleanup(true);
		td = pd.totalDifficulty + tdIncrease;

#if ETH_PARANOIA
		checkConsistency();
#endif
		// All ok - insert into DB
		{
			WriteGuard l(x_details);
			m_details[newHash] = BlockDetails((uint)pd.number + 1, td, bi.parentHash, {}, b);
			m_details[bi.parentHash].children.push_back(newHash);
		}
		{
			WriteGuard l(x_blooms);
			m_blooms[newHash] = bb;
		}
		{
			WriteGuard l(x_traces);
			m_traces[newHash] = bt;
		}

		m_extrasDB->Put(m_writeOptions, toSlice(newHash), (ldb::Slice)eth::ref(m_details[newHash].rlp()));
		m_extrasDB->Put(m_writeOptions, toSlice(bi.parentHash), (ldb::Slice)eth::ref(m_details[bi.parentHash].rlp()));
		m_extrasDB->Put(m_writeOptions, toSlice(newHash, 1), (ldb::Slice)eth::ref(m_blooms[newHash].rlp()));
		m_extrasDB->Put(m_writeOptions, toSlice(newHash, 2), (ldb::Slice)eth::ref(m_traces[newHash].rlp()));
		m_db->Put(m_writeOptions, toSlice(newHash), (ldb::Slice)ref(_block));

#if ETH_PARANOIA
		checkConsistency();
#endif
	}
#if ETH_CATCH
	catch (Exception const& _e)
	{
		clog(BlockChainNote) << "   Malformed block (" << _e.description() << ").";
		throw;
	}
#endif

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
		clog(BlockChainNote) << "   Imported and best. Has" << (details(bi.parentHash).children.size() - 1) << "siblings. Route:";
		for (auto r: ret)
			clog(BlockChainNote) << r;
	}
	else
	{
		clog(BlockChainNote) << "   Imported but not best (oTD:" << details(last).totalDifficulty << ", TD:" << td << ")";
	}
	return ret;
}

h256s BlockChain::treeRoute(h256 _from, h256 _to, h256* o_common) const
{
	h256s ret;
	h256s back;
	unsigned fn = details(_from).number;
	unsigned tn = details(_to).number;
	while (fn > tn)
	{
		ret.push_back(_from);
		_from = details(_from).parent;
		fn--;
	}
	while (fn < tn)
	{
		back.push_back(_to);
		_to = details(_to).parent;
		tn--;
	}
	while (_from != _to)
	{
		_from = details(_from).parent;
		_to = details(_to).parent;
		ret.push_back(_from);
		back.push_back(_to);
	}
	if (o_common)
		*o_common = _from;
	ret.reserve(ret.size() + back.size());
	for (auto it = back.cbegin(); it != back.cend(); ++it)
		ret.push_back(*it);
	return ret;
}

void BlockChain::checkConsistency()
{
	m_details.clear();
	ldb::Iterator* it = m_db->NewIterator(m_readOptions);
	for (it->SeekToFirst(); it->Valid(); it->Next())
		if (it->key().size() == 32)
		{
			h256 h((byte const*)it->key().data(), h256::ConstructFromPointer);
			auto dh = details(h);
			auto p = dh.parent;
			if (p != h256())
			{
				auto dp = details(p);
				assert(contains(dp.children, h));
				assert(dp.number == dh.number - 1);
			}
		}
	delete it;
}

bytes BlockChain::block(h256 _hash) const
{
	if (_hash == m_genesisHash)
		return m_genesisBlock;

	{
		ReadGuard l(x_cache);
		auto it = m_cache.find(_hash);
		if (it != m_cache.end())
			return it->second;
	}

	string d;
	m_db->Get(m_readOptions, ldb::Slice((char const*)&_hash, 32), &d);

	WriteGuard l(x_cache);
	m_cache[_hash].resize(d.size());
	memcpy(m_cache[_hash].data(), d.data(), d.size());

	if (!d.size())
		cwarn << "Couldn't find requested block:" << _hash;

	return m_cache[_hash];
}

h256 BlockChain::numberHash(unsigned _n) const
{
	if (!_n)
		return genesisHash();
	h256 ret = currentHash();
	for (; _n < details().number; ++_n, ret = details(ret).parent) {}
	return ret;
}