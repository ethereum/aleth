// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include "EthashProofOfWork.h"

#include <libethereum/GenericMiner.h>

namespace dev
{
namespace eth
{
class EthashCPUMiner : public GenericMiner<EthashProofOfWork>
{
public:
    explicit EthashCPUMiner(GenericMiner<EthashProofOfWork>::ConstructionInfo const& _ci);
    ~EthashCPUMiner() override;

    static unsigned instances()
    {
        return s_numInstances > 0 ? s_numInstances : std::thread::hardware_concurrency();
    }
    static std::string platformInfo();
    static void setNumInstances(unsigned _instances)
    {
        s_numInstances = std::min<unsigned>(_instances, std::thread::hardware_concurrency());
    }

protected:
    void kickOff() override;
    void pause() override;

private:
    static unsigned s_numInstances;

    void startWorking();
    void stopWorking();
    void minerBody();

    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_shouldStop;
};
}  // namespace eth
}  // namespace dev
