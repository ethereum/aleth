#pragma once

#include <string>

namespace dev
{

namespace test
{

std::string wast2wasm(std::string const& input, bool debug = false);

}

}
