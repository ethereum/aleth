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
/** @file BlockChainInsert.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>, Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * BlockChain test functions.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libethcore/BasicAuthority.h>
#include <libethereum/BlockChain.h>
#include <libethereum/Block.h>
#include <libethereum/GenesisInfo.h>
#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/BlockChainHelper.h>
using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {

namespace test {

BOOST_FIXTURE_TEST_SUITE(BlockChainInsertTests, TestOutputHelper)

class TestClient
{
public:
	TestClient(Secret const& _authority):
		m_path(boost::filesystem::temp_directory_path().string() + "/" + toString(chrono::system_clock::now().time_since_epoch().count()) + "-" + FixedHash<4>::random().hex()),
		m_stateDB(State::openDB(m_path, h256())),
		m_bc(eth::ChainParams(c_genesisInfoTestBasicAuthority), m_path)
	{
		sealer()->setOption("authority", rlp(_authority.makeInsecure()));
		sealer()->setOption("authorities", rlpList(toAddress(_authority)));
		sealer()->onSealGenerated([&](bytes const& sealedHeader){
			sealed = sealedHeader;
		});
	}

	void seal(Block& _block)
	{
		cdebug << "seal";
		sealed = bytes();
		cdebug << "commiting...";
		_block.commitToSeal(m_bc);
		cdebug << (void*)sealer();
		sealer()->generateSeal(_block.info());
		cdebug << toHex((bytes)sealed);
		sealed.waitNot({});
		cdebug << toHex((bytes)sealed);
		_block.sealBlock(sealed);
	}

	BlockChain& bc() { return m_bc; }
	OverlayDB& db() { return m_stateDB; }

	void sealAndImport(Block& _block)
	{
		seal(_block);
		cnote << "Importing sealed: " << sha3(sealed);
		cdebug << "importing..." << RLP(_block.blockData());
		m_bc.import(_block.blockData(), m_stateDB);
//		cdebug << "done.";
	}

	void import(Block const& _block)
	{
		m_bc.import(_block.blockData(), m_stateDB);
	}

	void insert(Block const& _block, BlockChain const& _bcSource)
	{
		BlockHeader bi(&_block.blockData());
		bytes receipts = _bcSource.receipts(bi.hash()).rlp();
		m_bc.insert(_block.blockData(), &receipts);
		assert(m_bc.isKnown(bi.hash(), false));
	}

private:
	SealEngineFace* sealer() const { return m_bc.sealEngine(); }

	string m_path;
	OverlayDB m_stateDB;
	BlockChain m_bc;
	Notified<bytes> sealed;
};

h256s subs(bytesConstRef _node)
{
	h256s ret;
	RLP r(_node);
	if (r.itemCount() == 17)
		// branch
	{
		for (RLP i: r)
			if (i.size() == 32)
				ret.push_back(i.toHash<h256>());
	}
	else if (r.itemCount() == 2)
		// extension or terminal
		if (r[1].size() == 32)	// TODO: check whether it's really an extension node, or whether it's just an terminal-node with 32 bytes of data.
			ret.push_back(r[1].toHash<h256>());
	// TODO: include
	return ret;
}

void syncStateTrie(bytesConstRef _block, OverlayDB const& _dbSource, OverlayDB& _dbDest)
{
	BlockHeader bi(_block);
	cnote << "Root is " << bi.stateRoot();

	h256s todo = {bi.stateRoot()};
	vector<bytes> data;
	while (!todo.empty())
	{
		h256 h = todo.back();
		todo.pop_back();
		bytes d = asBytes(_dbSource.lookup(h));
		cnote << h << ": " << RLP(&d);
		auto s = subs(&d);
		cnote << "   More: " << s;
		todo += s;
		// push final value.
		data.push_back(d);
	}
	for (auto const& d: data)
	{
		cnote << "Inserting " << sha3(d);
		_dbDest.insert(sha3(d), &d);
	}
}

BOOST_AUTO_TEST_CASE(bcBasicInsert)
{
	BasicAuthority::init();
	BasicAuthority::init();

	if (g_logVerbosity != -1)
		g_logVerbosity = 4;

	KeyPair me = Secret(sha3("Gav Wood"));
	KeyPair myMiner = Secret(sha3("Gav's Miner"));

	TestClient tcFull(me.secret());
	TestClient tcLight(me.secret());

	Block block = tcFull.bc().genesisBlock(tcFull.db());
	block.setAuthor(myMiner.address());

	// Sync up - this won't do much until we use the last state.
	block.sync(tcFull.bc());

	// Seal and import into full client.
	cnote << "First seal and import";
	tcFull.sealAndImport(block);

	// Insert into light client.
	cnote << "Insert into light";
	tcLight.insert(block, tcFull.bc());

	// Sync light client's state trie.
	cnote << "Syncing light state";
	syncStateTrie(&block.blockData(), tcFull.db(), tcLight.db());

	// Mine another block. Importing into both should work now.

	// Prep block for a transaction.
	cnote << "Prep block";
	block.sync(tcFull.bc());
	cnote << block.state();
	while (utcTime() < block.info().timestamp())
		this_thread::sleep_for(chrono::milliseconds(100));

	// Inject a transaction to transfer funds from miner to me.
	Transaction t(1000, 10000, 100000, me.address(), bytes(), block.transactionsFrom(myMiner.address()), myMiner.secret());
	assert(t.sender() == myMiner.address());
	cnote << "Execute transaction";
	block.execute(tcFull.bc().lastHashes(), t);
	cnote << block.state();

	// Seal and import into both.
	cnote << "Seal and import";
	tcFull.sealAndImport(block);
	cnote << "Import into light";
	tcLight.import(block);

	cnote << tcFull.bc();
	cnote << tcLight.bc();
	block.sync(tcFull.bc());

	cnote << block.state();
	cnote << tcFull.bc().dumpDatabase();
	cnote << tcLight.bc().dumpDatabase();
	BOOST_REQUIRE_EQUAL(tcFull.bc().dumpDatabase(), tcLight.bc().dumpDatabase());
}

BOOST_AUTO_TEST_SUITE_END()

}
}

