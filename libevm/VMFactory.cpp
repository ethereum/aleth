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

#include <libaleth-interpreter/interpreter.h>

#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

#if ETH_EVMJIT
#include <evmjit.h>
#endif

#if ETH_HERA
#include <hera.h>
#endif

namespace dll = boost::dll;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

using evmc_create_fn = evmc_instance*();

namespace dev
{
namespace eth
{
namespace
{
auto g_kind = VMKind::Legacy;

/// The pointer to EVMC create function in DLL EVMC VM.
///
/// This variable is only written once when processing command line arguments,
/// so access is thread-safe.
boost::function<evmc_create_fn> g_dllEvmcCreate;

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

boost::function<evmc_create_fn> loadDllVM(fs::path _path)
{
    if (!fs::exists(_path))
    {
        // This we report error as "invalid value for option --vm". This is better than reporting
        // DLL loading error in case someone tries to use build-time disabled VM like "jit".
        BOOST_THROW_EXCEPTION(po::validation_error(
            po::validation_error::invalid_option_value, "vm", _path.string(), 1));
    }

    auto symbols = dll::library_info{_path}.symbols();
    static const auto predicate = [](const std::string& symbol) {
        return symbol.find("evmc_create_") == 0;
    };
    auto it = std::find_if(symbols.begin(), symbols.end(), predicate);

    if (it == symbols.end())
    {
        // This is what boost is doing when symbol not found.
        auto ec = std::make_error_code(std::errc::invalid_seek);
        std::string what = "loading " + _path.string() + " failed: EVMC create function not found";
        BOOST_THROW_EXCEPTION(std::system_error(ec, what));
    }

    // Check for ambiguity of EVMC create functions.
    auto dupIt = std::find_if(std::next(it), symbols.end(), predicate);
    if (dupIt != symbols.end())
    {
        auto ec = std::make_error_code(std::errc::invalid_seek);
        std::string what = "loading " + _path.string() +
                           " failed: multiple EVMC create functions: " + *it + " and " + *dupIt;
        BOOST_THROW_EXCEPTION(std::system_error(ec, what));
    }
    return dll::import<evmc_create_fn>(_path, *it);
}

void setVMKind(const std::string& _name)
{
    for (auto& entry : vmKindsTable)
    {
        // Try to find a match in the table of VMs.
        if (_name == entry.name)
        {
            g_kind = entry.kind;
            return;
        }
    }

    g_dllEvmcCreate = loadDllVM(_name);
    g_kind = VMKind::DLL;
}
}  // namespace

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

    po::options_description opts("VM OPTIONS", _lineLength);
    auto add = opts.add_options();

    add("vm",
        po::value<std::string>()
            ->value_name("<name>|<path>")
            ->default_value("legacy")
            ->notifier(setVMKind),
        description.data());

    add(c_evmcPrefix,
        po::value<std::vector<std::string>>()
            ->value_name("<option>=<value>")
            ->notifier(parseEvmcOptions),
        "EVM-C option\n");

    return opts;
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
        return std::unique_ptr<VMFace>(new EVMC{evmc_create_hera()});
#endif
    case VMKind::Interpreter:
        return std::unique_ptr<VMFace>(new EVMC{evmc_create_interpreter()});
    case VMKind::DLL:
        return std::unique_ptr<VMFace>(new EVMC{g_dllEvmcCreate()});
    case VMKind::Legacy:
    default:
        return std::unique_ptr<VMFace>(new LegacyVM);
    }
}
}
}
