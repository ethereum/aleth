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
/** @file EthashClient.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <tuple>
#include <libethereum/Client.h>

namespace dev
{
namespace eth
{

class Ethash;

DEV_SIMPLE_EXCEPTION(InvalidSealEngine);

class EthashClient: public Client
{
public:
	/// Trivial forwarding constructor.
	EthashClient(
		ChainParams const& _params,
		int _networkID,
		p2p::Host* _host,
		std::shared_ptr<GasPricer> _gpForAdoption,
		std::string const& _dbPath = std::string(),
		WithExisting _forceAction = WithExisting::Trust,
		TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}
	);

	Ethash* ethash() const;

	/// Enable/disable precomputing of the DAG for next epoch
	void setShouldPrecomputeDAG(bool _precompute);

	/// Are we mining now?
	bool isMining() const;

	/// The hashrate...
	u256 hashrate() const;

	/// Check the progress of the mining.
	WorkingProgress miningProgress() const;

	/// @returns true only if it's worth bothering to prep the mining block.
	bool shouldServeWork() const { return m_bq.items().first == 0 && (isMining() || remoteActive()); }

	/// Update to the latest transactions and get hash of the current block to be mined minus the
	/// nonce (the 'work hash') and the difficulty to be met.
	/// @returns Tuple of hash without seal, seed hash, target boundary.
	std::tuple<h256, h256, h256> getEthashWork();

	/** @brief Submit the proof for the proof-of-work.
	 * @param _s A valid solution.
	 * @return true if the solution was indeed valid and accepted.
	 */
	bool submitEthashWork(h256 const& _mixHash, h64 const& _nonce);

	void submitExternalHashrate(u256 const& _rate, h256 const& _id);

protected:
	u256 externalHashrate() const;

	// external hashrate
	mutable std::unordered_map<h256, std::pair<u256, std::chrono::steady_clock::time_point>> m_externalRates;
	mutable SharedMutex x_externalRates;
};

EthashClient& asEthashClient(Interface& _c);
EthashClient* asEthashClient(Interface* _c);

}
}
