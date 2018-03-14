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
#include "VM.h"
#include <evmjit.h>
#include <hera.h>

#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/function.hpp>

namespace dll = boost::dll;
namespace po = boost::program_options;

namespace dev
{
namespace eth
{
VMFactory::StaticData VMFactory::staticData;

namespace
{
auto g_kind = VMKind::Interpreter;
}

void validate(boost::any& v, const std::vector<std::string>& values, VMKind* /* target_type */, int)
{
    // Make sure no previous assignment to 'v' was made.
    po::validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    for (auto& entry : VMFactory::staticData.vmKindsTable)
    {
        // Try to find a match in the table of VMs.
        if (s == entry.second)
        {
            v = entry.first;
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
        for (auto& entry : VMFactory::staticData.vmKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.second;
        }

        return "Select VM implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("VM Options", _lineLength);
    auto add = opts.add_options();

    add("vm",
        po::value<VMKind>()
            ->value_name("<name>")
            ->default_value(VMKind::Interpreter, "interpreter")
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
    case VMKind::Hera:
    {
        boost::function<struct evm_instance*()> create_vm =
            dll::import<struct evm_instance*()>(
                "hera",
                "hera_create",
                dll::load_mode::search_system_folders | dll::load_mode::append_decorations
            );
        return std::unique_ptr<VMFace>(new EVMC{create_vm()});
    }
    case VMKind::Interpreter:
    default:
        return std::unique_ptr<VMFace>(new VM);
    }
}

VMFactory::StaticData::StaticData() {
    vmKindsTable[VMKind::Interpreter] = "interpreter";
#if ETH_EVMJIT
    vmKindsTable[VMKind::JIT] = "jit";
#endif
    try {
        dll::shared_library lib(
            "hera",
            dll::load_mode::append_decorations
        );
        if(lib.has("hera_create")) {
            vmKindsTable[VMKind::Hera] = "hera";
        }
    } catch( const boost::system::system_error& ex ) {
        if(ex.code() == boost::system::errc::bad_file_descriptor) {
            // We couldn't find the plugin, that's ok
        } else {
            throw;
        }
    }
}
}
}
