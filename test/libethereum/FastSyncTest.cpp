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

#include <libethereum/EthereumPeer.h>
#include <libethereum/FastSync.h>
#include <test/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace
{

class MockEthereumPeer : public EthereumPeerFace
{
public:
	/// Peer's latest block's hash that we know about or default null value if no need to sync.
	h256 latestHash() const override { return m_latestHash; }
	/// Peer's latest block's total difficulty.
	u256 totalDifficulty() const override { return m_totalDifficulty; }

	bool validateStatus(h256 const& /*_genesisHash*/, vector<unsigned> const& /*_protocolVersions*/, u256 const& /*_networkId*/) override { return true; }
		
	void requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned /*_skip*/, bool /*_reverse*/) override
	{
		m_startHashRequested = _startHash;
		m_countRequested = _count;
	}
	void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned /*_skip*/, bool /*_reverse*/) override
	{
		m_startNumberRequested = _startNumber;
		m_countRequested = _count;
	}

	/// Request specified blocks from peer.
	void requestBlockBodies(h256s const& /*_blocks*/) override {}

	/// Request values for specified keys from peer.
	void requestNodeData(h256s const& /*_hashes*/) override {}

	/// Request receipts for specified blocks from peer.
	void requestReceipts(h256s const& /*_blocks*/) override {}

	void addRating(int _r) override { m_rating += _r; }

	h256 m_latestHash;
	u256 m_totalDifficulty;

	h256 m_startHashRequested;
	unsigned m_startNumberRequested = 0;
	unsigned m_countRequested = 0;
	int m_rating = 0;
};

bytes createHeaderData(unsigned _blockNumber)
{
	BlockHeader header;
	header.setNumber(_blockNumber);
	RLPStream headerRLP;
	header.streamRLP(headerRLP, WithSeal);
	
	RLPStream headersRLP;
	headersRLP.appendList(headerRLP);

	return headersRLP.out();
}

pair<bytes, h256s> createHeadersDataAndHashes(const vector<unsigned>& _blockNumbers, const h256& _parentHash = h256())
{
	RLPStream headersRLP(_blockNumbers.size());
	h256s hashes;

	h256 prevHash = _parentHash;
	for (unsigned blockNumber : _blockNumbers)
	{
		BlockHeader header;
		header.setNumber(blockNumber);
		header.setParentHash(prevHash);

		RLPStream headerRLP;
		header.streamRLP(headerRLP, WithSeal);

		headersRLP.append(RLP(headerRLP.out()));

		prevHash = header.hash();
		hashes.push_back(prevHash);
	}

	return make_pair(headersRLP.out(), hashes);
}

bytes createHeadersData(const vector<unsigned>& _blockNumbers, const h256& _parentHash = h256())
{
	bytes ret;
	tie(ret, ignore) = createHeadersDataAndHashes(_blockNumbers, _parentHash);
	return ret;
}

bool mapContainsAll(std::map<unsigned, bytes> _map, const vector<unsigned>& _numbers)
{
	for (unsigned number : _numbers)
		if (_map.find(number) == _map.end())
			return false;

	return true;
}

bool mapContainsNone(std::map<unsigned, bytes> _map, const vector<unsigned>& _numbers)
{
	for (unsigned number : _numbers)
		if (_map.find(number) != _map.end())
			return false;
	
	return true;
}

}

class FastSyncFixture: public TestOutputHelper
{
public:
	FastSyncFixture():
		peer(make_shared<MockEthereumPeer>()),
		sync(h256("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3") , {62, 63}, 2)
	{
	}

	shared_ptr<MockEthereumPeer> peer;
	FastSync sync;
};

BOOST_FIXTURE_TEST_SUITE(FastSyncSuite, FastSyncFixture)

BOOST_AUTO_TEST_CASE(FastSyncSuite_requestsHighestBlock)
{
	peer->m_totalDifficulty = 100;
	peer->m_latestHash = h256("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef");
	sync.onPeerStatus(peer);

	BOOST_REQUIRE_EQUAL(peer->m_startHashRequested, peer->m_latestHash);
	BOOST_REQUIRE_EQUAL(peer->m_countRequested, 1);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_doesntRequestsHighestBlockIfAlreadySeen)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	shared_ptr<MockEthereumPeer> peer2(make_shared<MockEthereumPeer>());
	peer2->m_totalDifficulty = 50;
	sync.onPeerStatus(peer2);

	BOOST_REQUIRE_EQUAL(peer2->m_countRequested, 0);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_requestsTheFirstRange)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	BOOST_REQUIRE_EQUAL(peer->m_startNumberRequested, 0);
	BOOST_REQUIRE_EQUAL(peer->m_countRequested, 1024);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_newHighestBlockExpandsHeadersToDownload)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	shared_ptr<MockEthereumPeer> peer2(make_shared<MockEthereumPeer>());
	peer2->m_totalDifficulty = 150;
	sync.onPeerStatus(peer2);

	bytes header1(createHeaderData(512));
	sync.onPeerBlockHeaders(peer, RLP(header1));

	bytes header2(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer2, RLP(header2));

	BOOST_REQUIRE_EQUAL(peer2->m_startNumberRequested, 513);
	BOOST_REQUIRE_EQUAL(peer2->m_countRequested, 1024);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_higestBlockIsSaved)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(1024));
	sync.onPeerBlockHeaders(peer, RLP(header));

	BOOST_REQUIRE(sync.downloadedHeaders().find(1024) != sync.downloadedHeaders().end());
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_downloadedHeadersAreSaved)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers(createHeadersData({ 0, 1, 2 }));
	sync.onPeerBlockHeaders(peer, RLP(headers));

	std::map<unsigned, bytes> const& downloadedHeaders = sync.downloadedHeaders();
	BOOST_REQUIRE(mapContainsAll(downloadedHeaders, {0, 1, 2}));
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_requestsNextRange)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers(createHeadersData({ 0, 1, 2 }));
	sync.onPeerBlockHeaders(peer, RLP(headers));

	BOOST_REQUIRE_EQUAL(peer->m_startNumberRequested, 3);
	BOOST_REQUIRE_EQUAL(peer->m_countRequested, 1024);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_requestsTheSameRangeIfTimeouted)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	sync.onPeerRequestTimeout(peer, Asking::BlockHeaders);

	shared_ptr<MockEthereumPeer> peer2(make_shared<MockEthereumPeer>());
	peer2->m_totalDifficulty = 50;
	sync.onPeerStatus(peer2);

	BOOST_REQUIRE_EQUAL(peer2->m_startNumberRequested, 0);
	BOOST_REQUIRE_EQUAL(peer2->m_countRequested, 1024);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_ignoresNotExpectedHeaders)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers(createHeadersData({ 1025, 1026, 1027 }));
	sync.onPeerBlockHeaders(peer, RLP(headers));

	std::map<unsigned, bytes> const& downloadedHeaders = sync.downloadedHeaders();
	BOOST_REQUIRE(mapContainsNone(downloadedHeaders, { 1025, 1026, 1027 }));

	BOOST_REQUIRE_LT(peer->m_rating, 0);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_ignoresHeadersWithIncorrectParentHash)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers1(createHeadersData({ 0, 1, 2 }));
	sync.onPeerBlockHeaders(peer, RLP(headers1));

	bytes headers2(createHeadersData({ 3, 4, 5 }));
	sync.onPeerBlockHeaders(peer, RLP(headers2));

	std::map<unsigned, bytes> const& downloadedHeaders = sync.downloadedHeaders();
	BOOST_REQUIRE(mapContainsNone(downloadedHeaders, { 3, 4, 5 }));

	BOOST_REQUIRE_LT(peer->m_rating, 0);
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_savesHeadersWithCorrectParentHash)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers1;
	h256s hashes1;
	tie(headers1, hashes1) = createHeadersDataAndHashes({ 0, 1, 2 });
	sync.onPeerBlockHeaders(peer, RLP(headers1));

	bytes headers2(createHeadersData({ 3, 4, 5 }, hashes1.back()));
	sync.onPeerBlockHeaders(peer, RLP(headers2));

	std::map<unsigned, bytes> const& downloadedHeaders = sync.downloadedHeaders();
	BOOST_REQUIRE(mapContainsAll(downloadedHeaders, { 3, 4, 5 }));
}

BOOST_AUTO_TEST_CASE(FastSyncSuite_deletesOlderHeadersWithIncorrectParentHash)
{
	peer->m_totalDifficulty = 100;
	sync.onPeerStatus(peer);

	bytes header(createHeaderData(2048));
	sync.onPeerBlockHeaders(peer, RLP(header));

	bytes headers1(createHeadersData({ 0, 1, 2 }));
	bytes headers2(createHeadersData({ 3, 4, 5 })); // incorrect parent hash for block 3
	bytes headers3(createHeadersData({ 10, 11, 12 })); // incorrect parent hash for block 10, but we won't know that until we receive 9

	sync.onPeerBlockHeaders(peer, RLP(headers3));
	sync.onPeerBlockHeaders(peer, RLP(headers2));
	sync.onPeerBlockHeaders(peer, RLP(headers1));

	std::map<unsigned, bytes> const& downloadedHeaders = sync.downloadedHeaders();
	BOOST_REQUIRE(mapContainsAll(downloadedHeaders, { 0, 1, 2, 10, 11, 12 }));
	BOOST_REQUIRE(mapContainsNone(downloadedHeaders, { 3, 4, 5 }));
}

BOOST_AUTO_TEST_SUITE_END()
