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
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * RLP tool.
 */
#include <clocale>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;

void help()
{
	cout
		<< "Usage rlp <mode> [OPTIONS]" << endl
		<< "Modes:" << endl
		<< "    create <json>  Given a simplified JSON string, output the RLP." << endl
		<< "    render [ <file> | -- ]  Render the given RLP. Options:" << endl
		<< "      --indent <string>  Use string as the level indentation (default '  ')." << endl
		<< "      --hex-ints  Render integers in hex." << endl
		<< "      --string-ints  Render integers in the same way as strings." << endl
		<< "      --ascii-strings  Render data as C-style strings or hex depending on content being ASCII." << endl
		<< "      --force-string  Force all data to be rendered as C-style strings." << endl
		<< "      --force-escape  When rendering as C-style strings, force all characters to be escaped." << endl
		<< "      --force-hex  Force all data to be rendered as raw hex." << endl
		<< "    list [ <file> | -- ]  List the items in the RLP list by hash and size." << endl
		<< "    extract [ <file> | -- ]  Extract all items in the RLP list, named by hash." << endl
		<< "    assemble [ <manifest> | <base path> ] <file> ...  Given a manifest & files, output the RLP." << endl
		<< "      -D,--dapp  Dapp-building mode; equivalent to --encrypt --64." << endl
		<< endl
		<< "General options:" << endl
		<< "    -e,--encrypt  Encrypt the RLP data prior to output." << endl
		<< "    -L,--lenience  Try not to bomb out early if possible." << endl
		<< "    -x,--hex,--base-16  Treat input RLP as hex encoded data." << endl
		<< "    -k,--keccak  Output Keccak-256 hash only." << endl
		<< "    --64,--base-64  Treat input RLP as base-64 encoded data." << endl
		<< "    -b,--bin,--base-256  Treat input RLP as raw binary data." << endl
		<< "    -q,--quiet  Don't place additional information on stderr." << endl
		<< "    -h,--help  Print this help message and exit." << endl
		<< "    -V,--version  Show the version and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "rlp version " << dev::Version << endl;
	exit(0);
}

/*
The equivalent of setlocale(LC_ALL, “C”) is called before any user code is run.
If the user has an invalid environment setting then it is possible for the call
to set locale to fail, so there are only two possible actions, the first is to
throw a runtime exception and cause the program to quit (default behaviour),
or the second is to modify the environment to something sensible (least
surprising behaviour).

The follow code produces the least surprising behaviour. It will use the user
specified default locale if it is valid, and if not then it will modify the
environment the process is running in to use a sensible default. This also means
that users do not need to install language packs for their OS.
*/
void setDefaultOrCLocale()
{
#if __unix__
	if (!std::setlocale(LC_ALL, ""))
	{
		setenv("LC_ALL", "C", 1);
	}
#endif
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
		bool hexPrefix = true;
		bool forceString = false;
		bool escapeAll = false;
		bool forceHex = true;
	};

	RLPStreamer(ostream& _out, Prefs _p): m_out(_out), m_prefs(_p) {}

	void output(RLP const& _d, unsigned _level = 0)
	{
		if (_d.isNull())
			m_out << "null";
		else if (_d.isInt() && !m_prefs.stringInts)
			if (m_prefs.hexInts)
				m_out << (m_prefs.hexPrefix ? "0x" : "") << toHex(toCompactBigEndian(_d.toInt<bigint>(RLP::LaissezFaire), 1), 1);
			else
				m_out << _d.toInt<bigint>(RLP::LaissezFaire);
		else if (_d.isData() || (_d.isInt() && m_prefs.stringInts))
			if (m_prefs.forceString || (!m_prefs.forceHex && isAscii(_d.toString())))
				m_out << escaped(_d.toString(), m_prefs.escapeAll);
			else
				m_out << "\"" << (m_prefs.hexPrefix ? "0x" : "") << toHex(_d.toBytes()) << "\"";
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

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "render")
			mode = Mode::Render;
		else if (arg == "create")
			mode = Mode::Create;
		else if ((arg == "-i" || arg == "--indent") && i + 1 < argc)
			prefs.indent = argv[++i];
		else if (arg == "--hex-ints")
			prefs.hexInts = true;
		else if (arg == "--string-ints")
			prefs.stringInts = true;
		else if (arg == "--ascii-strings")
			prefs.forceString = prefs.forceHex = false;
		else if (arg == "--force-string")
			prefs.forceString = true;
		else if (arg == "--force-hex")
			prefs.forceHex = true, prefs.forceString = false;
		else if (arg == "--force-escape")
			prefs.escapeAll = true;
		else if (arg == "-n" || arg == "--nice")
			prefs.forceString = true, prefs.stringInts = false, prefs.forceHex = false, prefs.indent = "  ";
		else if (arg == "list")
			mode = Mode::ListArchive;
		else if (arg == "extract")
			mode = Mode::ExtractArchive;
		else if (arg == "assemble")
			mode = Mode::AssembleArchive;
		else if (arg == "-L" || arg == "--lenience")
			lenience = true;
		else if (arg == "-D" || arg == "--dapp")
			encrypt = true, encoding = Encoding::Base64;
		else if (arg == "-V" || arg == "--version")
			version();
		else if (arg == "-q" || arg == "--quiet")
			quiet = true;
		else if (arg == "-x" || arg == "--hex" || arg == "--base-16")
			encoding = Encoding::Hex;
		else if (arg == "-k" || arg == "--keccak")
			encoding = Encoding::Keccak;
		else if (arg == "--64" || arg == "--base-64")
			encoding = Encoding::Base64;
		else if (arg == "-b" || arg == "--bin" || arg == "--base-256")
			encoding = Encoding::Binary;
		else if (arg == "-e" || arg == "--encrypt")
			encrypt = true;
		else if (inputFile.empty())
			inputFile = arg;
		else
			otherInputs.push_back(arg);
	}

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
//						cerr << "'" << b << "':" << (int)b << endl;
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
				cout << "Error: Invalid format; RLP data is not a list." << endl;
				exit(1);
			}
			cout << rlp.itemCount() << " items:" << endl;
			for (auto i: rlp)
			{
				if (!i.isData())
				{
					cout << "Error: Invalid format; RLP list item is not data." << endl;
					if (!lenience)
						exit(1);
				}
				cout << "    " << i.size() << " bytes: " << sha3(i.data()) << endl;
			}
			break;
		}
		case Mode::ExtractArchive:
		{
			if (!rlp.isList())
			{
				cout << "Error: Invalid format; RLP data is not a list." << endl;
				exit(1);
			}
			cout << rlp.itemCount() << " items:" << endl;
			for (auto i: rlp)
			{
				if (!i.isData())
				{
					cout << "Error: Invalid format; RLP list item is not data." << endl;
					if (!lenience)
						exit(1);
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
				exit(1);
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
						exit(1);
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
		exit(1);
	}

	return 0;
}
