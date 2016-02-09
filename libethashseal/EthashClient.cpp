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
/** @file EthashClient.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "EthashClient.h"
#include "Ethash.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

EthashClient& dev::eth::asEthashClient(Interface& _c)
{
	if (dynamic_cast<Ethash*>(_c.sealEngine()))
		return dynamic_cast<EthashClient&>(_c);
	throw InvalidSealEngine();
}

EthashClient* dev::eth::asEthashClient(Interface* _c)
{
	if (dynamic_cast<Ethash*>(_c->sealEngine()))
		return &dynamic_cast<EthashClient&>(*_c);
	throw InvalidSealEngine();
}

DEV_SIMPLE_EXCEPTION(ChainParamsNotEthash);

EthashClient::EthashClient(
	ChainParams const& _params,
	int _networkID,
	p2p::Host* _host,
	std::shared_ptr<GasPricer> _gpForAdoption,
	std::string const& _dbPath,
	WithExisting _forceAction,
	TransactionQueue::Limits const& _limits
):
	Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _forceAction, _limits)
{
	// will throw if we're not an Ethash seal engine.
	asEthashClient(*this);
}

Ethash* EthashClient::ethash() const
{
	return dynamic_cast<Ethash*>(Client::sealEngine());
}

bool EthashClient::isMining() const
{
	return ethash()->farm().isMining();
}

WorkingProgress EthashClient::miningProgress() const
{
	if (isMining())
		return ethash()->farm().miningProgress();
	return WorkingProgress();
}

u256 EthashClient::hashrate() const
{
	u256 r = externalHashrate();
	if (isMining())
		r += miningProgress().rate();
	return r;
}

std::tuple<h256, h256, h256> EthashClient::getEthashWork()
{
	// lock the work so a later submission isn't invalidated by processing a transaction elsewhere.
	// this will be reset as soon as a new block arrives, allowing more transactions to be processed.
	bool oldShould = shouldServeWork();
	m_lastGetWork = chrono::system_clock::now();

	if (!sealEngine()->shouldSeal(this))
		return std::tuple<h256, h256, h256>();

	// if this request has made us bother to serve work, prep it now.
	if (!oldShould && shouldServeWork())
		onPostStateChanged();
	else
		// otherwise, set this to true so that it gets prepped next time.
		m_remoteWorking = true;
	ethash()->manuallySetWork(m_sealingInfo);
	return std::tuple<h256, h256, h256>(m_sealingInfo.hash(WithoutSeal), Ethash::seedHash(m_sealingInfo), Ethash::boundary(m_sealingInfo));
}

bool EthashClient::submitEthashWork(h256 const& _mixHash, h64 const& _nonce)
{
	ethash()->manuallySubmitWork(_mixHash, _nonce);
	return true;
}

void EthashClient::setShouldPrecomputeDAG(bool _precompute)
{
	bytes trueBytes {1};
	bytes falseBytes {0};
	sealEngine()->setOption("precomputeDAG", _precompute ? trueBytes: falseBytes);
}

void EthashClient::submitExternalHashrate(u256 const& _rate, h256 const& _id)
{
	WriteGuard(x_externalRates);
	m_externalRates[_id] = make_pair(_rate, chrono::steady_clock::now());
}

u256 EthashClient::externalHashrate() const
{
	u256 ret = 0;
	WriteGuard(x_externalRates);
	for (auto i = m_externalRates.begin(); i != m_externalRates.end();)
		if (chrono::steady_clock::now() - i->second.second > chrono::seconds(5))
			i = m_externalRates.erase(i);
		else
			ret += i++->second.first;
	return ret;
}
