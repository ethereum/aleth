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
#include "SmartVM.h"
#include "VM.h"
#include <libdevcore/Assertions.h>
#include <evmjit.h>

namespace po = boost::program_options;

namespace dev
{
namespace eth
{
namespace
{
auto g_kind = VMKind::Interpreter;

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
#if ETH_EVMJIT
    {VMKind::JIT, "jit"},
    {VMKind::Smart, "smart"},
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
    opts.add_options()("vm",
        po::value<VMKind>()
            ->value_name("<name>")
            ->default_value(VMKind::Interpreter, "interpreter")
            ->notifier(VMFactory::setKind),
        description.data());

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
#if ETH_EVMJIT
    switch (_kind)
    {
    default:
    case VMKind::Interpreter:
        return std::unique_ptr<VMFace>(new VM);
    case VMKind::JIT:
        return std::unique_ptr<VMFace>(new EVMC{evmjit_create()});
    case VMKind::Smart:
        return std::unique_ptr<VMFace>(new SmartVM);
    }
#else
    asserts(_kind == VMKind::Interpreter && "JIT disabled in build configuration");
    return std::unique_ptr<VMFace>(new VM);
#endif
}
}
}
