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

#include "VMFace.h"

#include <boost/program_options/options_description.hpp>

namespace dev
{
namespace eth
{
enum class VMKind
{
    Interpreter,
    Legacy,
    DLL
};

/// Returns the EVMC options parsed from command line.
std::vector<std::pair<std::string, std::string>>& evmcOptions() noexcept;

/// Provide a set of program options related to VMs.
///
/// @param _lineLength  The line length for description text wrapping, the same as in
///                     boost::program_options::options_description::options_description().
boost::program_options::options_description vmProgramOptions(
    unsigned _lineLength = boost::program_options::options_description::m_default_line_length);

using VMPtr = std::unique_ptr<VMFace, void (*)(VMFace*)>;

class VMFactory
{
public:
    VMFactory() = delete;
    ~VMFactory() = delete;

    /// Creates a VM instance of the global kind (controlled by the --vm command line option).
    static VMPtr create();

    /// Creates a VM instance of the kind provided.
    static VMPtr create(VMKind _kind);
};
}  // namespace eth
}  // namespace dev
