// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Base64.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>
#include <libethcore/Common.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <clocale>
#include <fstream>
#include <iostream>
using namespace std;
using namespace dev;
using namespace dev::eth;

namespace js = json_spirit;
namespace po = boost::program_options;

void version()
{
    cout << "rlp version " << dev::Version << endl;
    exit(0);
}

enum class Mode {
    AssembleArchive,
    ListArchive,
    ExtractArchive,
    Render,
    Create
};

enum class Encoding {
    Auto,
    Hex,
    Base64,
    Binary,
    Keccak,
};

bool isAscii(string const& _s)
{
    // Always hex-encode anything beginning with 0x to avoid ambiguity.
    if (_s.size() >= 2 && _s.substr(0, 2) == "0x")
        return false;

    for (char c: _s)
        if (c < 32)
            return false;
    return true;
}

class RLPStreamer
{
public:
    struct Prefs
    {
        string indent;
        bool hexInts = false;
        bool stringInts = true;
        bool forceString = false;
        bool escapeAll = false;
        bool forceHex = true;
    };

    RLPStreamer(ostream& _out, Prefs _p): m_out(_out), m_prefs(_p)
    {
        if (_p.hexInts)
            _out << std::hex << std::showbase << std::nouppercase;
    }

    void output(RLP const& _d, unsigned _level = 0)
    {
        if (_d.isNull())
            m_out << "null";
        else if (_d.isInt() && !m_prefs.stringInts)
            m_out << _d.toInt<bigint>(RLP::LaissezFaire);
        else if (_d.isData() || (_d.isInt() && m_prefs.stringInts))
            if (m_prefs.forceString || (!m_prefs.forceHex && isAscii(_d.toString())))
                m_out << escaped(_d.toString(), m_prefs.escapeAll);
            else
                m_out << "\"" << toHexPrefixed(_d.toBytes()) << "\"";
        else if (_d.isList())
        {
            m_out << "[";
            string newline = "\n";
            for (unsigned i = 0; i < _level + 1; ++i)
                newline += m_prefs.indent;
            int j = 0;
            for (auto i: _d)
            {
                m_out << (j++ ?
                    (m_prefs.indent.empty() ? ", " : ("," + newline)) :
                    (m_prefs.indent.empty() ? " " : newline));
                output(i, _level + 1);
            }
            newline = newline.substr(0, newline.size() - m_prefs.indent.size());
            m_out << (m_prefs.indent.empty() ? (j ? " ]" : "]") : (j ? newline + "]" : "]"));
        }
    }

private:
    std::ostream& m_out;
    Prefs m_prefs;
};

void putOut(bytes _out, Encoding _encoding, bool _encrypt, bool _quiet)
{
    dev::h256 h = dev::sha3(_out);
    if (_encrypt)
        crypto::Secp256k1PP::get()->encrypt(toPublic(Secret(h)), _out);
    if (!_quiet)
        cerr << "Keccak of RLP: " << h.hex() << endl;

    switch (_encoding)
    {
    case Encoding::Hex: case Encoding::Auto:
        cout << toHex(_out) << endl;
        break;
    case Encoding::Base64:
        cout << toBase64(&_out) << endl;
        break;
    case Encoding::Binary:
        cout.write((char const*)_out.data(), _out.size());
        break;
    case Encoding::Keccak:
        cout << sha3(_out).hex() << endl;
        break;
    }
}

int main(int argc, char** argv)
{
    setDefaultOrCLocale();
    Encoding encoding = Encoding::Auto;
    Mode mode = Mode::Render;
    string inputFile;
    strings otherInputs;
    bool lenience = false;
    bool quiet = false;
    bool encrypt = false;
    RLPStreamer::Prefs prefs;

    po::options_description renderOptions("Render options");
    auto addRenderOption = renderOptions.add_options();
    addRenderOption("indent,i", po::value<string>()->implicit_value("  "),
        "<string>  Use string as the level indentation (default '  ').");
    addRenderOption("hex-ints", "Render integers in hex.");
    addRenderOption("string-ints", "Render integers in the same way as strings.");
    addRenderOption(
        "ascii-strings", "Render data as C-style strings or hex depending on content being ASCII.");
    addRenderOption("force-string", "Force all data to be rendered as C-style strings.");
    addRenderOption(
        "force-escape", "When rendering as C-style strings, force all characters to be escaped.");
    addRenderOption("force-hex", "Force all data to be rendered as raw hex.");

    po::options_description generalOptions("General options");
    auto addGeneralOption = generalOptions.add_options();
    addGeneralOption("dapp,D", "Dapp-building mode; equivalent to --encrypt --base-64.");
    addGeneralOption("encrypt,e", "Encrypt the RLP data prior to output.");
    addGeneralOption("lenience,L", "Try not to bomb out early if possible.");
    addGeneralOption("hex,x", "Treat input RLP as hex encoded data.");
    addGeneralOption("keccak,k", "Output Keccak-256 hash only.");
    addGeneralOption("base-64,6", "Treat input RLP as base-64 encoded data.");
    addGeneralOption("bin,b", "Treat input RLP as raw binary data.");
    addGeneralOption("quiet,q", "Don't place additional information on stderr.");
    addGeneralOption("help,h", "Print this help message and exit.");
    addGeneralOption("version,V", "Show the version and exit.");

    po::options_description allowedOptions("Allowed options");
    allowedOptions.add(generalOptions).add(renderOptions);

    po::variables_map vm;
    vector<string> unrecognisedOptions;
    try
    {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(allowedOptions).allow_unregistered().run();
        unrecognisedOptions = collect_unrecognized(parsed.options, po::include_positional);
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::error const& e)
    {
        cerr << e.what();
        return AlethErrors::ArgumentProcessingFailure;
    }


    for (size_t i = 0; i < unrecognisedOptions.size(); ++i)
    {
        string arg =  unrecognisedOptions[i];
        if (arg == "render")
            mode = Mode::Render;
        else if (arg == "create")
            mode = Mode::Create;
        else if (arg == "list")
            mode = Mode::ListArchive;
        else if (arg == "extract")
            mode = Mode::ExtractArchive;
        else if (arg == "assemble")
            mode = Mode::AssembleArchive;
        else if (inputFile.empty())
            inputFile = arg;
        else
            otherInputs.push_back(arg);
    }
    if (vm.count("help")) {
        cout << "Usage rlp <mode> [OPTIONS]\nModes:\n"
             << "    create   <json>  Given a simplified JSON string, output the RLP." << endl
             << "    render   [ <file> | -- ]  Render the given RLP." << endl
             << "    list     [ <file> | -- ]  List the items in the RLP list by hash and size." << endl
             << "    extract  [ <file> | -- ]  Extract all items in the RLP list, named by hash." << endl
             << "    assemble [ <manifest> | <base path> ] <file> ...  Given a manifest & files, output the RLP." << endl
             << renderOptions << generalOptions;
        exit(AlethErrors::Success);
    }
    if (vm.count("lenience"))
        lenience = true;
    if (vm.count("dapp"))
        encrypt = true, encoding = Encoding::Base64;
    if (vm.count("version"))
        version();
    if (vm.count("quiet"))
        quiet = true;
    if (vm.count("hex"))
        encoding = Encoding::Hex;
    if (vm.count("keccak"))
        encoding = Encoding::Keccak;
    if (vm.count("base-64"))
        encoding = Encoding::Base64;
    if (vm.count("bin"))
        encoding = Encoding::Binary;
    if (vm.count("encrypt"))
        encrypt = true;
    if (vm.count("indent"))
        prefs.indent = vm["indent"].as<string>();
    if (vm.count("hex-ints"))
        prefs.hexInts = true;
    if (vm.count("string-ints"))
        prefs.stringInts = true;
    if (vm.count("ascii-strings"))
        prefs.forceString = prefs.forceHex = false;
    if (vm.count("force-string"))
        prefs.forceString = true;
    if (vm.count("force-hex"))
        prefs.forceHex = true, prefs.forceString = false;
    if (vm.count("force-escape"))
        prefs.escapeAll = true;

    bytes in;
    if (inputFile == "--")
        for (int i = cin.get(); i != -1; i = cin.get())
            in.push_back((byte)i);
    else if (boost::filesystem::is_regular_file(inputFile))
        in = contents(inputFile);
    else
        in = asBytes(inputFile);

    bytes b;

    if (mode != Mode::Create && mode != Mode::AssembleArchive)
    {
        if (encoding == Encoding::Auto)
        {
            encoding = Encoding::Hex;
            for (char b: in)
                if (b != '\n' && b != ' ' && b != '\t')
                {
                    if (encoding == Encoding::Hex && (b < '0' || b > '9' ) && (b < 'a' || b > 'f' ) && (b < 'A' || b > 'F' ))
                    {
//                      cerr << "'" << b << "':" << (int)b << endl;
                        encoding = Encoding::Base64;
                    }
                    if (encoding == Encoding::Base64 && (b < '0' || b > '9' ) && (b < 'a' || b > 'z' ) && (b < 'A' || b > 'Z' ) && b != '+' && b != '/')
                    {
                        encoding = Encoding::Binary;
                        break;
                    }
                }
        }
        switch (encoding)
        {
        case Encoding::Hex:
        {
            string s = asString(in);
            boost::algorithm::replace_all(s, " ", "");
            boost::algorithm::replace_all(s, "\n", "");
            boost::algorithm::replace_all(s, "\t", "");
            b = fromHex(s);
            break;
        }
        case Encoding::Base64:
        {
            string s = asString(in);
            boost::algorithm::replace_all(s, " ", "");
            boost::algorithm::replace_all(s, "\n", "");
            boost::algorithm::replace_all(s, "\t", "");
            b = fromBase64(s);
            break;
        }
        default:
            swap(b, in);
            break;
        }
    }

    try
    {
        RLP rlp(b);
        switch (mode)
        {
        case Mode::ListArchive:
        {
            if (!rlp.isList())
            {
                cerr << "Error: Invalid format; RLP data is not a list." << endl;
                exit(AlethErrors::BadRlp);
            }
            cout << rlp.itemCount() << " items:" << endl;
            for (auto i: rlp)
            {
                if (!i.isData())
                {
                    cerr << "Error: Invalid format; RLP list item is not data." << endl;
                    if (!lenience)
                        exit(AlethErrors::BadRlp);
                }
                cout << "    " << i.size() << " bytes: " << sha3(i.data()) << endl;
            }
            break;
        }
        case Mode::ExtractArchive:
        {
            if (!rlp.isList())
            {
                cerr << "Error: Invalid format; RLP data is not a list." << endl;
                exit(AlethErrors::BadRlp);
            }
            cout << rlp.itemCount() << " items:" << endl;
            for (auto i: rlp)
            {
                if (!i.isData())
                {
                    cerr << "Error: Invalid format; RLP list item is not data." << endl;
                    if (!lenience)
                        exit(AlethErrors::BadRlp);
                }
                ofstream fout;
                fout.open(toString(sha3(i.data())));
                fout.write(reinterpret_cast<char const*>(i.data().data()), i.data().size());
            }
            break;
        }
        case Mode::AssembleArchive:
        {
            if (boost::filesystem::is_directory(inputFile))
            {
                js::mArray entries;
                auto basePath = boost::filesystem::canonical(boost::filesystem::path(inputFile)).string();
                for (string& i: otherInputs)
                {
                    js::mObject entry;
                    strings parsed;
                    boost::algorithm::split(parsed, i, boost::is_any_of(","));
                    i = parsed[0];
                    for (unsigned j = 1; j < parsed.size(); ++j)
                    {
                        strings nv;
                        boost::algorithm::split(nv, parsed[j], boost::is_any_of(":"));
                        if (nv.size() == 2)
                            entry[nv[0]] = nv[1];
                        else{} // TODO: error
                    }
                    if (!entry.count("path"))
                    {
                        std::string path = boost::filesystem::canonical(boost::filesystem::path(parsed[0])).string().substr(basePath.size());
                        if (path == "/index.html")
                            path = "/";
                        entry["path"] = path;
                    }
                    entry["hash"] = toHex(dev::sha3(contents(parsed[0])).ref());
                    entries.push_back(entry);
                }
                js::mObject o;
                o["entries"] = entries;
                auto os = js::write_string(js::mValue(o), false);
                in = asBytes(os);
            }

            strings addedInputs;
            for (auto i: otherInputs)
                if (!boost::filesystem::is_regular_file(i))
                    cerr << "Skipped " << i << std::endl;
                else
                    addedInputs.push_back(i);

            RLPStream r(addedInputs.size() + 1);
            r.append(in);
            for (auto i: addedInputs)
                r.append(contents(i));
            putOut(r.out(), encoding, encrypt, quiet);
            break;
        }
        case Mode::Render:
        {
            RLPStreamer s(cout, prefs);
            s.output(rlp);
            cout << endl;
            break;
        }
        case Mode::Create:
        {
            vector<js::mValue> v(1);
            try {
                js::read_string(asString(in), v[0]);
            }
            catch (...)
            {
                cerr << "Error: Invalid format; bad JSON." << endl;
                exit(AlethErrors::InvalidJson);
            }
            RLPStream out;
            while (!v.empty())
            {
                auto vb = v.back();
                v.pop_back();
                switch (vb.type())
                {
                case js::array_type:
                {
                    js::mArray a = vb.get_array();
                    out.appendList(a.size());
                    for (int i = a.size() - 1; i >= 0; --i)
                        v.push_back(a[i]);
                    break;
                }
                case js::str_type:
                {
                    string const& s = vb.get_str();
                    if (s.size() >= 2 && s.substr(0, 2) == "0x")
                        out << fromHex(s);
                    else
                    {
                        // assume it's a normal JS escaped string.
                        bytes ss;
                        ss.reserve(s.size());
                        for (unsigned i = 0; i < s.size(); ++i)
                            if (s[i] == '\\' && i + 1 < s.size())
                            {
                                if (s[++i] == 'x' && i + 2 < s.size())
                                    ss.push_back(fromHex(s.substr(i, 2))[0]);
                            }
                            else if (s[i] != '\\')
                                ss.push_back((byte)s[i]);
                        out << ss;
                    }
                    break;
                }
                case js::int_type:
                    out << vb.get_int();
                    break;
                default:
                    cerr << "ERROR: Unsupported type in JSON." << endl;
                    if (!lenience)
                        exit(AlethErrors::UnsupportedJsonType);
                }
            }
            putOut(out.out(), encoding, encrypt, quiet);
            break;
        }
        default:;
        }
    }
    catch (...)
    {
        cerr << "Error: Invalid format; bad RLP." << endl;
        exit(AlethErrors::BadRlp);
    }

    return AlethErrors::Success;
}
