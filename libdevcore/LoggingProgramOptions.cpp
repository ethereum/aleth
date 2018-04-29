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

#include "LoggingProgramOptions.h"

namespace po = boost::program_options;

namespace dev
{
po::options_description createLoggingProgramOptions(unsigned _lineLength, LoggingOptions& _options)
{
    po::options_description optionsDescr("LOGGING OPTIONS", _lineLength);
    auto addLoggingOption = optionsDescr.add_options();
    addLoggingOption("log-verbosity,v", po::value<int>(&_options.verbosity)->value_name("<0 - 4>"),
        "Set the log verbosity from 0 to 4 (default: 2).");
    addLoggingOption("log-channels",
        po::value<std::vector<std::string>>(&_options.includeChannels)
            ->value_name("<channel_list>")
            ->multitoken(),
        "Space-separated list of the log channels to show (default: show all channels).");
    addLoggingOption("log-exclude-channels",
        po::value<std::vector<std::string>>(&_options.excludeChannels)
            ->value_name("<channel_list>")
            ->multitoken(),
        "Space-separated list of the log channels to hide.\n");

    return optionsDescr;
}

}  // namespace dev
