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
/** @file EthashCPUMiner.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Determines the PoW algorithm.
 */

#include "EthashCPUMiner.h"
#include "Ethash.h"

#include <ethash/ethash.hpp>

#include <thread>
#include <chrono>
#include <random>

using namespace std;
using namespace dev;
using namespace eth;

unsigned EthashCPUMiner::s_numInstances = 0;


EthashCPUMiner::EthashCPUMiner(GenericMiner<EthashProofOfWork>::ConstructionInfo const& _ci)
  : GenericMiner<EthashProofOfWork>(_ci)
{
}

EthashCPUMiner::~EthashCPUMiner()
{
    stopWorking();
}

void EthashCPUMiner::kickOff()
{
    stopWorking();
    startWorking();
}

void EthashCPUMiner::pause()
{
    stopWorking();
}

void EthashCPUMiner::startWorking()
{
    if (!m_thread)
    {
        m_shouldStop = false;
        m_thread.reset(new thread(&EthashCPUMiner::minerBody, this));
    }
}

void EthashCPUMiner::stopWorking()
{
    if (m_thread)
    {
        m_shouldStop = true;
        m_thread->join();
        m_thread.reset();
    }
}


void EthashCPUMiner::minerBody()
{
    setThreadName("miner" + toString(index()));

    auto tid = std::this_thread::get_id();
    static std::mt19937_64 s_eng((utcTime() + std::hash<decltype(tid)>()(tid)));

    uint64_t tryNonce = s_eng();

    // FIXME: Use epoch number, not seed hash in the work package.
    WorkPackage w = work();

    int epoch = ethash::find_epoch_number(toEthash(w.seedHash));
    auto& ethashContext = ethash::get_global_epoch_context_full(epoch);

    h256 boundary = w.boundary;
    for (unsigned hashCount = 1; !m_shouldStop; tryNonce++, hashCount++)
    {
        auto result = ethash::hash(ethashContext, toEthash(w.headerHash()), tryNonce);
        h256 value = h256(result.final_hash.bytes, h256::ConstructFromPointer);
        if (value <= boundary && submitProof(EthashProofOfWork::Solution{(h64)(u64)tryNonce,
                                     h256(result.mix_hash.bytes, h256::ConstructFromPointer)}))
            break;
        if (!(hashCount % 100))
            accumulateHashes(100);
    }
}

std::string EthashCPUMiner::platformInfo()
{
    string baseline = toString(std::thread::hardware_concurrency()) + "-thread CPU";
    return baseline;
}
