#pragma once

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
/** @file
 * CLI module for mining.
 */

#include <boost/algorithm/string/case_conv.hpp>
#include <libdevcore/CommonJS.h>
#include <libethcore/BasicAuthority.h>
#include <libethcore/Exceptions.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/EthashCPUMiner.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

bool isTrue(std::string const& _m)
{
    return _m == "on" || _m == "yes" || _m == "true" || _m == "1";
}

bool isFalse(std::string const& _m)
{
    return _m == "off" || _m == "no" || _m == "false" || _m == "0";
}

class BadArgument: public Exception {};

class MinerCLI
{
public:
    enum class OperationMode
    {
        None,
        Benchmark
    };


    explicit MinerCLI(OperationMode _mode = OperationMode::None): mode(_mode)
    {
        Ethash::init();
        NoProof::init();
        BasicAuthority::init();
    }

    bool interpretOption(size_t& i, vector<string> const& argv)
    {
        size_t argc = argv.size();
        string arg = argv[i];
        if (arg == "--benchmark-warmup" && i + 1 < argc)
            try {
                m_benchmarkWarmup = stol(argv[++i]);
            }
            catch (...)
            {
                cerr << "Bad " << arg << " option: " << argv[i] << endl;
                BOOST_THROW_EXCEPTION(BadArgument());
            }
        else if (arg == "--benchmark-trial" && i + 1 < argc)
            try {
                m_benchmarkTrial = stol(argv[++i]);
            }
            catch (...)
            {
                cerr << "Bad " << arg << " option: " << argv[i] << endl;
                BOOST_THROW_EXCEPTION(BadArgument());
            }
        else if (arg == "--benchmark-trials" && i + 1 < argc)
            try {
                m_benchmarkTrials = stol(argv[++i]);
            }
            catch (...)
            {
                cerr << "Bad " << arg << " option: " << argv[i] << endl;
                BOOST_THROW_EXCEPTION(BadArgument());
            }
        else if (arg == "-C" || arg == "--cpu")
            m_minerType = "cpu";
        else if (arg == "--current-block" && i + 1 < argc)
            m_currentBlock = stol(argv[++i]);
        else if (arg == "-M" || arg == "--benchmark")
            mode = OperationMode::Benchmark;
        else if ((arg == "-t" || arg == "--mining-threads") && i + 1 < argc)
        {
            try {
                m_miningThreads = stol(argv[++i]);
            }
            catch (...)
            {
                cerr << "Bad " << arg << " option: " << argv[i] << endl;
                BOOST_THROW_EXCEPTION(BadArgument());
            }
        }
        else
            return false;
        return true;
    }

    void execute()
    {
        if (m_minerType == "cpu")
            EthashCPUMiner::setNumInstances(m_miningThreads);
        else if (mode == OperationMode::Benchmark)
            doBenchmark(m_minerType, m_benchmarkWarmup, m_benchmarkTrial, m_benchmarkTrials);
    }

    static void streamHelp(ostream& _out)
    {
        _out << "BENCHMARKING MODE:\n"
             << "  -M,--benchmark               Benchmark for mining and exit\n"
             << "  --benchmark-warmup <seconds> Set the duration of warmup for the benchmark tests "
                "(default: 3)\n"
             << "  --benchmark-trial <seconds>  Set the duration for each trial for the benchmark "
                "tests (default: 3)\n"
             << "  --benchmark-trials <n>       Set the number of trials for the benchmark tests "
                "(default: 5)\n\n"

             << "MINING CONFIGURATION:\n"
             << "  -C,--cpu                   When mining, use the CPU\n"
             << "  -t, --mining-threads <n>   Limit number of CPU/GPU miners to n (default: use "
                "everything available on selected platform)\n"
             << "  --current-block            Let the miner know the current block number at "
                "configuration time. Will help determine DAG size and required GPU memory\n"
             << "  --disable-submit-hashrate  When mining, don't submit hashrate to node\n\n";
    }

    std::string minerType() const { return m_minerType; }

private:
    void doBenchmark(std::string const& _m, unsigned _warmupDuration = 15, unsigned _trialDuration = 3, unsigned _trials = 5)
    {
        BlockHeader genesis;
        genesis.setDifficulty(1 << 18);
        cdebug << Ethash::boundary(genesis);

        GenericFarm<EthashProofOfWork> f;
        map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
        sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
        f.setSealers(sealers);
        f.onSolutionFound([&](EthashProofOfWork::Solution) { return false; });

        string platformInfo = EthashCPUMiner::platformInfo();
        cout << "Benchmarking on platform: " << platformInfo << endl;

        genesis.setDifficulty(u256(1) << 63);
        f.setWork(genesis);
        f.start(_m);

        map<u256, WorkingProgress> results;
        u256 mean = 0;
        u256 innerMean = 0;
        for (unsigned i = 0; i <= _trials; ++i)
        {
            if (!i)
                cout << "Warming up..." << endl;
            else
                cout << "Trial " << i << "... " << flush;
            this_thread::sleep_for(chrono::seconds(i ? _trialDuration : _warmupDuration));

            auto mp = f.miningProgress();
            f.resetMiningProgress();
            if (!i)
                continue;
            auto rate = mp.rate();

            cout << rate << endl;
            results[rate] = mp;
            mean += rate;
        }
        f.stop();
        int j = -1;
        for (auto const& r: results)
            if (++j > 0 && j < (int)_trials - 1)
                innerMean += r.second.rate();
        innerMean /= (_trials - 2);
        cout << "min/mean/max: " << results.begin()->second.rate() << "/" << (mean / _trials) << "/" << results.rbegin()->second.rate() << " H/s" << endl;
        cout << "inner mean: " << innerMean << " H/s" << endl;
        exit(0);
    }

    /// Operating mode.
    OperationMode mode;

    /// Mining options
    std::string m_minerType = "cpu";
    unsigned m_miningThreads = UINT_MAX;
    uint64_t m_currentBlock = 0;

    /// Benchmarking params
    unsigned m_benchmarkWarmup = 3;
    unsigned m_benchmarkTrial = 3;
    unsigned m_benchmarkTrials = 5;
};
