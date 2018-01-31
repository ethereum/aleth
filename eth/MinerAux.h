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
/** @file MinerAux.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * CLI module for mining.
 */

#include <boost/algorithm/string/case_conv.hpp>
#include <libdevcore/CommonJS.h>
#include <libethcore/BasicAuthority.h>
#include <libethcore/Exceptions.h>
#include <libethashseal/EthashCPUMiner.h>

// TODO - having using derivatives in header files is very poor style, and we need to fix these up.
//
// http://stackoverflow.com/questions/4872373/why-is-including-using-namespace-into-a-header-file-a-bad-idea-in-c
// 
// "However you'll virtually never see a using directive in a header file (at least not outside of scope).
// The reason is that using directive eliminate the protection of that particular namespace, and the effect last
// until the end of current compilation unit. If you put a using directive (outside of a scope) in a header file,
// it means that this loss of "namespace protection" will occur within any file that include this header,
// which often mean other header files."
//
// Bob has already done just that in https://github.com/bobsummerwill/cpp-ethereum/commits/cmake_fixes/ethminer,
// and we should apply those changes back to 'develop'.  It is probably best to defer that cleanup
// until after attempting to backport the Genoil ethminer changes, because they are fairly extensive
// in terms of lines of change, though all the changes are just adding explicit namespace prefixes.
// So let's start with just the subset of changes which minimizes the #include dependencies.
//
// See https://github.com/bobsummerwill/cpp-ethereum/commit/53af845268b91bc6aa1dab53a6eac675157a072b
// See https://github.com/bobsummerwill/cpp-ethereum/commit/3b9e581d7c04c637ebda18d3d86b5a24d29226f4
//
// More generally, the fact that nearly all of the code for ethminer is actually in the 'MinerAux.h'
// header file, rather than in a source file, is also poor style and should probably be addressed.
// Perhaps there is some historical reason for this which I am unaware of?

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

inline std::string credits()
{
	std::ostringstream out;
	out
		<< "cpp-ethereum " << dev::Version << endl
		<< "  By cpp-ethereum contributors, (c) 2013-2018." << endl
		<< "  See the README for contributors and credits." << endl;
	return out.str();
}

class BadArgument: public Exception {};
struct MiningChannel: public LogChannel
{
	static const char* name() { return EthGreen "miner"; }
	static const int verbosity = 2;
	static const bool debug = false;
};
#define minelog clog(MiningChannel)

class MinerCLI
{
public:
	enum class OperationMode
	{
		None,
		DAGInit,
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
		else if (arg == "--no-precompute")
		{
			m_precompute = false;
		}
		else if ((arg == "-D" || arg == "--create-dag") && i + 1 < argc)
		{
			string m = boost::to_lower_copy(string(argv[++i]));
			mode = OperationMode::DAGInit;
			try
			{
				m_initDAG = stol(m);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << m << endl;
				BOOST_THROW_EXCEPTION(BadArgument());
			}
		}
		else if ((arg == "-w" || arg == "--check-pow") && i + 4 < argc)
		{
			string m;
			try
			{
				BlockHeader bi;
				m = boost::to_lower_copy(string(argv[++i]));
				h256 powHash(m);
				m = boost::to_lower_copy(string(argv[++i]));
				h256 seedHash;
				if (m.size() == 64 || m.size() == 66)
					seedHash = h256(m);
				else
					seedHash = EthashAux::seedHash(stol(m));
				m = boost::to_lower_copy(string(argv[++i]));
				bi.setDifficulty(u256(m));
				auto boundary = Ethash::boundary(bi);
				m = boost::to_lower_copy(string(argv[++i]));
				Ethash::setNonce(bi, h64(m));
				auto r = EthashAux::eval(seedHash, powHash, h64(m));
				bool valid = r.value < boundary;
				cout << (valid ? "VALID :-)" : "INVALID :-(") << endl;
				cout << r.value << (valid ? " < " : " >= ") << boundary << endl;
				cout << "  where " << boundary << " = 2^256 / " << bi.difficulty() << endl;
				cout << "  and " << r.value << " = ethash(" << powHash << ", " << h64(m) << ")" << endl;
				cout << "  with seed as " << seedHash << endl;
				if (valid)
					cout << "(mixHash = " << r.mixHash << ")" << endl;
				cout << "SHA3( light(seed) ) = " << sha3(EthashAux::light(Ethash::seedHash(bi))->data()) << endl;
				exit(0);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << m << endl;
				BOOST_THROW_EXCEPTION(BadArgument());
			}
		}
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
		if (mode == OperationMode::DAGInit)
			doInitDAG(m_initDAG);
		else if (mode == OperationMode::Benchmark)
			doBenchmark(m_minerType, m_benchmarkWarmup, m_benchmarkTrial, m_benchmarkTrials);
	}

	static void streamHelp(ostream& _out)
	{
		_out
			<< "Work farming mode:" << endl
			<< "    --no-precompute  Don't precompute the next epoch's DAG." << endl
			<< "Ethash verify mode:" << endl
			<< "    -w,--check-pow <headerHash> <seedHash> <difficulty> <nonce>  Check PoW credentials for validity." << endl
			<< endl
			<< "Benchmarking mode:" << endl
			<< "    -M,--benchmark  Benchmark for mining and exit; use with --cpu and --opencl." << endl
			<< "    --benchmark-warmup <seconds>  Set the duration of warmup for the benchmark tests (default: 3)." << endl
			<< "    --benchmark-trial <seconds>  Set the duration for each trial for the benchmark tests (default: 3)." << endl
			<< "    --benchmark-trials <n>  Set the number of trials for the benchmark tests (default: 5)." << endl
			<< "DAG creation mode:" << endl
			<< "    -D,--create-dag <number>  Create the DAG in preparation for mining on given block and exit." << endl
			<< "Mining configuration:" << endl
			<< "    -C,--cpu  When mining, use the CPU." << endl
			<< "    -t, --mining-threads <n> Limit number of CPU/GPU miners to n (default: use everything available on selected platform)" << endl
			<< "    --current-block Let the miner know the current block number at configuration time. Will help determine DAG size and required GPU memory." << endl
			<< "    --disable-submit-hashrate  When mining, don't submit hashrate to node." << endl;
	}

	std::string minerType() const { return m_minerType; }
	bool shouldPrecompute() const { return m_precompute; }

private:
	void doInitDAG(unsigned _n)
	{
		h256 seedHash = EthashAux::seedHash(_n);
		cout << "Initializing DAG for epoch beginning #" << (_n / 30000 * 30000) << " (seedhash " << seedHash.abridged() << "). This will take a while." << endl;
		EthashAux::full(seedHash, true);
		exit(0);
	}

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

		cout << "Preparing DAG..." << endl;
		Ethash::ensurePrecomputed(0);

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

	/// DAG initialisation param.
	unsigned m_initDAG = 0;

	/// Benchmarking params
	unsigned m_benchmarkWarmup = 3;
	unsigned m_benchmarkTrial = 3;
	unsigned m_benchmarkTrials = 5;

	bool m_precompute = true;
};
