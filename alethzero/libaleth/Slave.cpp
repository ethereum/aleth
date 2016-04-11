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
/** @file Slave.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Slave.h"
#include <QTimer>
#include <QTimerEvent>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonJS.h>
#include <libethashseal/Ethash.h>
#include "FarmClient.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

SlaveAux::SlaveAux():
	m_engine(make_shared<Ethash>()),
	m_id(h256::random())
{
	m_engine->farm().onSolutionFound([&](EthashProofOfWork::Solution const& sol) { return onSolutionFound(sol); });
}

SlaveAux::~SlaveAux()
{
}

bool SlaveAux::isWorking() const
{
	return m_engine->farm().isMining() && m_engine->farm().work();
}

void SlaveAux::start()
{
	if (!m_engine->farm().isMining())
	{
		m_engine->farm().setWork(EthashProofOfWork::WorkPackage());
		m_engine->farm().start(m_engine->sealer());
		QTimer::singleShot(0, this, [=](){
			this->m_workTimer = startTimer(500);
			this->m_rateTimer = startTimer(2000);
		});
	}
}

void SlaveAux::stop()
{
	if (m_engine->farm().isMining())
	{
		m_engine->farm().stop();
		QTimer::singleShot(0, this, [=](){
			killTimer(m_rateTimer);
			killTimer(m_workTimer);
		});
	}
}

void SlaveAux::timerEvent(QTimerEvent* _e)
{
	QString u = m_url;
	if (u.isEmpty())
		return;

	auto client = m_client;
	if (!client)
		m_client = client = make_shared<jsonrpc::HttpClient>(u.toStdString());

	::FarmClient rpc(*client);

	if (_e->timerId() == m_rateTimer)
	{
		// Submit hashrate
		auto mp = m_engine->farm().miningProgress();
		m_engine->farm().resetMiningProgress();
		m_hashrate = mp.rate();
		try
		{
			rpc.eth_submitHashrate(toJS(m_hashrate), "0x" + m_id.hex());
		}
		catch (jsonrpc::JsonRpcException const& _e)
		{
			cwarn << "Failed to submit hashrate.";
			cwarn << boost::diagnostic_information(_e);
		}
	}

	if (_e->timerId() == m_workTimer)
	{
		// Check for new work
		try
		{
			Json::Value v = rpc.eth_getWork();
			if (!v[0].asString().empty())
			{
				h256 newHeaderHash(v[0].asString());
				h256 newSeedHash(v[1].asString());
				if (!(m_dag = EthashAux::full(newSeedHash, true, [&](unsigned _pc){ onGeneratingDAG(_pc); return 0; })))
				{
					emit dagGenerationFailed();
					return;
				}

				if (m_precompute)
					EthashAux::computeFull(sha3(newSeedHash), true);

				if (newHeaderHash != m_current.headerHash)
				{
					m_current.headerHash = newHeaderHash;
					m_current.seedHash = newSeedHash;
					m_current.boundary = h256(fromHex(v[2].asString()), h256::AlignRight);
					onNewWork(m_current);
				}
			}
		}
		catch (jsonrpc::JsonRpcException&)
		{
			for (auto i = 3; --i; this_thread::sleep_for(chrono::seconds(1)))
				cerr << "JSON-RPC problem. Probably couldn't connect. Retrying in " << i << "... \r";
			cerr << endl;
		}
	}
}

void SlaveAux::setURL(QString _url)
{
	if (m_url != _url)
	{
		m_url = _url;
		m_client.reset();
	}
}

QString SlaveAux::sealer() const
{
	return QString::fromStdString(m_engine->sealer());
}

void SlaveAux::setSealer(QString _id)
{
	m_engine->setSealer(_id.toStdString());
	if (m_engine->farm().isMining())
	{
		stop();
		start();
	}
}

QStringList SlaveAux::sealers() const
{
	QStringList ret;
	for (auto const& i: m_engine->sealers())
		ret += QString::fromStdString(i);
	return ret;
}

void SlaveAux::onNewWork(eth::EthashProofOfWork::WorkPackage const& _work)
{
	cnote << "Got work package:";
	cnote << "  Header-hash:" << _work.headerHash.hex();
	cnote << "  Seedhash:" << _work.seedHash.hex();
	cnote << "  Target: " << h256(_work.boundary).hex();
	m_engine->farm().setWork(_work);
}

void SlaveAux::onGeneratingDAG(unsigned _pc)
{
	(void)_pc;
	emit generatingDAG(_pc);
}

bool SlaveAux::onSolutionFound(eth::EthashProofOfWork::Solution const& _sol)
{
	// TODO: Should be called from some other (main? worker?) thread?
	if (!reportSolution(_sol))
		return false;
	emit solutionFound(_sol);
	return true;
}

bool SlaveAux::reportSolution(eth::EthashProofOfWork::Solution const& _sol)
{
	try
	{
		if (EthashAux::eval(m_current.seedHash, m_current.headerHash, _sol.nonce).value < m_current.boundary)
		{
			cnote << "Solution found; Submitting to" << m_url.toStdString() << "...";
			cnote << "  Nonce:" << _sol.nonce.hex();
			cnote << "  Mixhash:" << _sol.mixHash.hex();
			cnote << "  Header-hash:" << m_current.headerHash.hex();
			cnote << "  Seedhash:" << m_current.seedHash.hex();
			cnote << "  Target: " << h256(m_current.boundary).hex();
			cnote << "  Ethash: " << h256(EthashAux::eval(m_current.seedHash, m_current.headerHash, _sol.nonce).value).hex();
			::FarmClient rpc(*m_client);
			bool ok = rpc.eth_submitWork("0x" + toString(_sol.nonce), "0x" + toString(m_current.headerHash), "0x" + toString(_sol.mixHash));
			if (ok)
				cnote << "B-) Submitted and accepted.";
			else
				cwarn << ":-( Not accepted.";
		}
		else
		{
			cwarn << "FAILURE: GPU gave incorrect result!";
			return false;
		}
	}
	catch (jsonrpc::JsonRpcException&)
	{
		cnote << ":-( Couldn't submit - JSONRPC connectivity problem.";
	}
	m_current.reset();
	return true;
}

Slave::Slave():
	m_aux(new SlaveAux)
{
	qRegisterMetaType<h256>();
	qRegisterMetaType<dev::eth::EthashProofOfWork::Solution>();
	qRegisterMetaType<dev::eth::EthashProofOfWork::WorkPackage>();
	m_aux->moveToThread(&m_worker);
	connect(m_aux, SIGNAL(solutionFound(eth::EthashProofOfWork::Solution)), SIGNAL(solutionFound(eth::EthashProofOfWork::Solution)));
	connect(m_aux, SIGNAL(generatingDAG(unsigned)), SIGNAL(generatingDAG(unsigned)));
	connect(m_aux, SIGNAL(dagGenerationFailed()), SIGNAL(dagGenerationFailed()));
	m_worker.start();
}

Slave::~Slave()
{
}
