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

#pragma once

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

#include <libethcore/Exceptions.h>

class BadArgument: public dev::Exception {};

class MinerCLI
{
public:
    enum class OperationMode
    {
        None,
        Benchmark
    };

    explicit MinerCLI(OperationMode _mode = OperationMode::None) : mode(_mode)
    {
    }

    void execute();

    std::string minerType() const { return m_minerType; }

    void interpretOptions(boost::program_options::variables_map const& _options);
    static boost::program_options::options_description createProgramOptions(unsigned int _lineLength);

private:
    void doBenchmark(std::string const& _m, unsigned _warmupDuration = 15, unsigned _trialDuration = 3, unsigned _trials = 5);

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
