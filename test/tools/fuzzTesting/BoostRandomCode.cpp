/*
    This file is part of aleth.

    aleth is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aleth is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "BoostRandomCode.h"

#include <random>
#include <string>

namespace dev
{
namespace test
{
BoostRandomCode::BoostRandomCode()
{
    percentDist = IntDistrib(0, 100);
    opCodeDist = IntDistrib(0, 255);
    opLengDist = IntDistrib(1, 32);
    opMemrDist = IntDistrib(0, 10485760);
    opSmallMemrDist = IntDistrib(0, 1024);
    uniIntDist = IntDistrib(0, 0x7fffffff);

    randOpCodeGen = std::bind(opCodeDist, gen);
    randOpLengGen = std::bind(opLengDist, gen);
    randOpMemrGen = std::bind(opMemrDist, gen);
    randoOpSmallMemrGen = std::bind(opSmallMemrDist, gen);
    randUniIntGen = std::bind(uniIntDist, gen);

    auto const& seedOption = Options::get().randomTestSeed;
    if (seedOption)
        gen.seed(*seedOption);
    else
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        unsigned int time = static_cast<unsigned int>(timeSinceEpoch);
        gen.seed(time);

        boost::filesystem::path debugFile(boost::filesystem::current_path());
        debugFile /= "randomCodeSeed.txt";
        writeFile(debugFile, asBytes(toString(time)));
    }
}

u256 BoostRandomCode::randomUniInt(u256 const& _minVal, u256 const& _maxVal)
{
    assert(_minVal <= _maxVal);
    std::uniform_int_distribution<uint64_t> uint64Dist{0, std::numeric_limits<uint64_t>::max()};
    u256 value = _minVal + (u256)uint64Dist(gen) % (_maxVal - _minVal);
    return value;
}

uint8_t BoostRandomCode::weightedOpcode(std::vector<int> const& _weights)
{
    DescreteDistrib opCodeProbability = DescreteDistrib{_weights.begin(), _weights.end()};
    return opCodeProbability(gen);
}
}
}
