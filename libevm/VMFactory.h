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
    JIT,
    Hera,
};

/// Returns the EVM-C options parsed from command line.
std::vector<std::pair<std::string, std::string>>& evmcOptions() noexcept;

/// Provide a set of program options related to VMs.
///
/// @param _lineLength  The line length for description text wrapping, the same as in
///                     boost::program_options::options_description::options_description().
boost::program_options::options_description vmProgramOptions(
    unsigned _lineLength = boost::program_options::options_description::m_default_line_length);

class VMFactory
{
	class StaticData
	{
		friend class VMFactory;

	public:
		/// The table of available VM implementations.
		std::map<VMKind, const char*> vmKindsTable;

		StaticData();
	};

public:
	VMFactory() = delete;
	~VMFactory() = delete;

	/// Does any static initialization that VMFactory needs, such as retrieving the list of
	/// available VM implementations at runtime.
	static StaticData staticData;

	/// Creates a VM instance of global kind (controlled by setKind() function).
	static std::unique_ptr<VMFace> create();

	/// Creates a VM instance of kind provided.
	static std::unique_ptr<VMFace> create(VMKind _kind);

	/// Set global VM kind
	static void setKind(VMKind _kind);
};

}
}
