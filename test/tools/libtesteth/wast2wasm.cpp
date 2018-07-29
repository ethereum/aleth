#include "wast2wasm.h"

#include <libdevcore/CommonData.h>

#include <boost/test/unit_test.hpp>

// Disable MSVC warning "unary minus operator applied to unsigned type".
#pragma warning(push)
#pragma warning(disable: 4146)
#include <wasm-binary.h>
#include <wasm-s-parser.h>
#include <wasm-validator.h>
#pragma warning(pop)

using namespace std;

namespace dev
{

namespace test
{

/* from https://github.com/ewasm/evm2wasm */
string wast2wasm(string const& input, bool debug)
{
    wasm::Module module;

    try {
        // FIXME: binaryen 1.37.28 actually modifies the input...
        //        as a workaround make a copy here
        string tmp = input;
        wasm::SExpressionParser parser(const_cast<char*>(tmp.c_str()));
        wasm::Element& root = *parser.root;
        wasm::SExpressionWasmBuilder builder(module, *root[0]);
    } catch (wasm::ParseException& p) {
        ostringstream err;
        p.dump(err);
        BOOST_ERROR("Error parsing wasm: " + err.str());
    }

    BOOST_REQUIRE_MESSAGE(wasm::WasmValidator().validate(module), "Compiled WASM module is not valid.");

    wasm::BufferWithRandomAccess buffer(debug);
    wasm::WasmBinaryWriter writer(&module, buffer, debug);
    writer.setNamesSection(false);
    writer.write();

    ostringstream output;
    buffer.writeTo(output);

    return toHexPrefixed(output.str());
}

}

}
