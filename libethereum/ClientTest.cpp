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
/** @file ClientTest.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2016
 */

#include <libethereum/EthereumHost.h>
#include <libethereum/ClientTest.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

ClientTest& dev::eth::asClientTest(Interface& _c)
{
	return dynamic_cast<ClientTest&>(_c);
}

ClientTest* dev::eth::asClientTest(Interface* _c)
{
	return &dynamic_cast<ClientTest&>(*_c);
}

ClientTest::ClientTest(
	ChainParams const& _params,
	int _networkID,
	p2p::Host* _host,
	std::shared_ptr<GasPricer> _gpForAdoption,
	std::string const& _dbPath,
	WithExisting _forceAction,
	TransactionQueue::Limits const& _limits
):
	Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _forceAction, _limits)
{}

void ClientTest::setChainParams(string const& _genesis)
{
	ChainParams params;
	try
	{
		params = params.loadConfig(_genesis);
		if (params.sealEngineName != "NoProof")
			BOOST_THROW_EXCEPTION(ChainParamsNotNoProof() << errinfo_comment("Provided configuration is not well formatted."));

		reopenChain(params, WithExisting::Kill);
		setAuthor(params.author); //for some reason author is not being set
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(ChainParamsInvalid() << errinfo_comment("Provided configuration is not well formatted."));
	}
}

bool ClientTest::addBlock(string const& _rlp)
{
	if (auto h = m_host.lock())
		h->noteNewBlocks();

	bytes rlpBytes = fromHex(_rlp, WhenError::Throw);
	RLP blockRLP(rlpBytes);
	return (m_bq.import(blockRLP.data(), true) == ImportResult::Success);
}

void ClientTest::modifyTimestamp(u256 const& _timestamp)
{
	Block block(chainParams().accountStartNonce);
	DEV_READ_GUARDED(x_preSeal)
		block = m_preSeal;

	Transactions transactions;
	DEV_READ_GUARDED(x_postSeal)
		transactions = m_postSeal.pending();
	block.resetCurrent(_timestamp);

	DEV_WRITE_GUARDED(x_preSeal)
		m_preSeal = block;

	auto lastHashes = bc().lastHashes();
	for (auto const& t: transactions)
		block.execute(lastHashes, t);

	DEV_WRITE_GUARDED(x_working)
		m_working = block;
	DEV_READ_GUARDED(x_postSeal)
		m_postSeal = block;

	onPostStateChanged();
}

void ClientTest::mineBlocks(unsigned _count)
{
	m_blocksToMine = _count;
	startSealing();
}

void ClientTest::onNewBlocks(h256s const& _blocks, h256Hash& io_changed)
{
	Client::onNewBlocks(_blocks, io_changed);

	if(--m_blocksToMine <= 0)
		stopSealing();
}

bool ClientTest::completeSync()
{
	auto h = m_host.lock();
	if (!h)
		return false;

	h->completeSync();
	return true;
}
