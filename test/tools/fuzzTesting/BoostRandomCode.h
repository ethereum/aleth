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

#pragma once

#include "RandomCode.h"
#include <random>

namespace dev
{
namespace test
{
using IntDistrib = std::uniform_int_distribution<>;
using DescreteDistrib = std::discrete_distribution<>;
using IntGenerator = std::function<int()>;

class BoostRandomCode : public RandomCodeBase
{
public:
    BoostRandomCode();

    /// Generate random
    u256 randomUniInt(u256 const& _minVal = 0,
        u256 const& _maxVal = std::numeric_limits<int64_t>::max()) override;
    int randomPercent() override { return percentDist(gen); }
    int randomSmallUniInt() override { return opMemrDist(gen); }
    int randomLength32() override { return randOpLengGen(); }
    int randomSmallMemoryLength() override { return randoOpSmallMemrGen(); }
    int randomMemoryLength() override { return randOpMemrGen(); }
    uint8_t randomOpcode() override { return randOpCodeGen(); }
    uint8_t weightedOpcode(std::vector<int> const& _weights) override;

private:
    std::mt19937_64 gen;         ///< Random generator
    IntDistrib opCodeDist;       ///< 0..255 opcodes
    IntDistrib percentDist;      ///< 0..100 percent
    IntDistrib opLengDist;       ///< 1..32  byte string
    IntDistrib opMemrDist;       ///< 1..10MB  byte string
    IntDistrib opSmallMemrDist;  /// 0..1kb
    IntDistrib uniIntDist;       ///< 0..0x7fffffff

    IntGenerator randUniIntGen;        ///< Generate random UniformInt from uniIntDist
    IntGenerator randOpCodeGen;        ///< Generate random value from opCodeDist
    IntGenerator randOpLengGen;        ///< Generate random length from opLengDist
    IntGenerator randOpMemrGen;        ///< Generate random length from opMemrDist
    IntGenerator randoOpSmallMemrGen;  ///< Generate random length from opSmallMemrDist
};
}
}
