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
/** @file ClientTest.h
 * @author Kholhlov Dimitry <dimitry@ethdev.com>
 * @date 2016
 */

#pragma once

#include <tuple>
#include <libethereum/Client.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace eth
{

DEV_SIMPLE_EXCEPTION(ChainParamsInvalid);
DEV_SIMPLE_EXCEPTION(ImportBlockFailed);

class ClientTest: public Client
{
public:
    enum BlockStatus
    {
        Idle,
        Mining,
        Error,
        Success
    };
    /// Trivial forwarding constructor.
	ClientTest(
		ChainParams const& _params,
		int _networkID,
		p2p::Host* _host,
		std::shared_ptr<GasPricer> _gpForAdoption,
		boost::filesystem::path const& _dbPath = boost::filesystem::path(),
		WithExisting _forceAction = WithExisting::Trust,
		TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}
	);
	~ClientTest();

	void setChainParams(std::string const& _genesis);
	void mineBlocks(unsigned _count);
	void modifyTimestamp(int64_t _timestamp);
	void rewindToBlock(unsigned _number);
    h256 importRawBlock(std::string const& _blockRLP);
    bool completeSync();
    BlockStatus getLastBlockStatus() const { return m_lastBlockStatus; }

protected:
	unsigned m_blocksToMine;
    BlockStatus m_lastBlockStatus;
    virtual void onNewBlocks(h256s const& _blocks, h256Hash& io_changed) override;
};

ClientTest& asClientTest(Interface& _c);
ClientTest* asClientTest(Interface* _c);

}
}
