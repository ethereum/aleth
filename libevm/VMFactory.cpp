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

#include "VMFactory.h"
#include "EVMC.h"
#include "LegacyVM.h"
#include "interpreter.h"
#include <evmjit.h>
#include <hera.h>

namespace po = boost::program_options;

namespace dev
{
namespace eth
{
namespace
{
auto g_kind = VMKind::Legacy;

/// A helper type to build the tabled of VM implementations.
///
/// More readable than std::tuple.
/// Fields are not initialized to allow usage of construction with initializer lists {}.
struct VMKindTableEntry
{
    VMKind kind;
    const char* name;
};

/// The table of available VM implementations.
///
/// We don't use a map to avoid complex dynamic initialization. This list will never be long,
/// so linear search only to parse command line arguments is not a problem.
VMKindTableEntry vmKindsTable[] = {
    {VMKind::Interpreter, "interpreter"},
    {VMKind::Legacy, "legacy"},
#if ETH_EVMJIT
    {VMKind::JIT, "jit"},
#endif
#if ETH_HERA
    {VMKind::Hera, "hera"},
#endif
};
}

void validate(boost::any& v, const std::vector<std::string>& values, VMKind* /* target_type */, int)
{
    // Make sure no previous assignment to 'v' was made.
    po::validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    for (auto& entry : vmKindsTable)
    {
        // Try to find a match in the table of VMs.
        if (s == entry.name)
        {
            v = entry.kind;
            return;
        }
    }

    throw po::validation_error(po::validation_error::invalid_option_value);
}

namespace
{
/// The name of the program option --evmc. The boost will trim the tailing
/// space and we can reuse this variable in exception message.
const char c_evmcPrefix[] = "evmc ";

/// The list of EVM-C options stored as pairs of (name, value).
std::vector<std::pair<std::string, std::string>> s_evmcOptions;

/// The additional parser for EVM-C options. The options should look like
/// `--evmc name=value` or `--evmc=name=value`. The boost pass the strings
/// of `name=value` here. This function splits the name and value or reports
/// the syntax error if the `=` character is missing.
void parseEvmcOptions(const std::vector<std::string>& _opts)
{
    for (auto& s : _opts)
    {
        auto separatorPos = s.find('=');
        if (separatorPos == s.npos)
            throw po::invalid_syntax{po::invalid_syntax::missing_parameter, c_evmcPrefix + s};
        auto name = s.substr(0, separatorPos);
        auto value = s.substr(separatorPos + 1);
        s_evmcOptions.emplace_back(std::move(name), std::move(value));
    }
}
}

std::vector<std::pair<std::string, std::string>>& evmcOptions() noexcept
{
    return s_evmcOptions;
};

po::options_description vmProgramOptions(unsigned _lineLength)
{
    // It must be a static object because boost expects const char*.
    static const std::string description = [] {
        std::string names;
        for (auto& entry : vmKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.name;
        }

        return "Select VM implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("VM Options", _lineLength);
    auto add = opts.add_options();

    add("vm",
        po::value<VMKind>()
            ->value_name("<name>")
            ->default_value(VMKind::Legacy, "legacy")
            ->notifier(VMFactory::setKind),
        description.data());

    add(c_evmcPrefix,
        po::value<std::vector<std::string>>()
            ->value_name("<option>=<value>")
            ->notifier(parseEvmcOptions),
        "EVM-C option");

    return opts;
}


void VMFactory::setKind(VMKind _kind)
{
    g_kind = _kind;
}

std::unique_ptr<VMFace> VMFactory::create()
{
    return create(g_kind);
}

std::unique_ptr<VMFace> VMFactory::create(VMKind _kind)
{
    switch (_kind)
    {
#ifdef ETH_EVMJIT
    case VMKind::JIT:
        return std::unique_ptr<VMFace>(new EVMC{evmjit_create()});
#endif
#ifdef ETH_HERA
    case VMKind::Hera:
        return std::unique_ptr<VMFace>(new EVMC{hera_create()});
#endif
    case VMKind::Interpreter:
        return std::unique_ptr<VMFace>(new EVMC{interpreter_create()});
    case VMKind::Legacy:
    default:
        return std::unique_ptr<VMFace>(new LegacyVM);
    }
}
}
}
