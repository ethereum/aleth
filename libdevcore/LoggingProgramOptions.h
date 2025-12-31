// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Log.h"
#include <boost/program_options/options_description.hpp>

namespace dev
{
boost::program_options::options_description createLoggingProgramOptions(
    unsigned _lineLength, LoggingOptions& _options, std::string const& _logChannels = {});

}  // namespace dev