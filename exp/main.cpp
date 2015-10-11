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
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Hash.h>
#include <libdevcore/Base58.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>
#include <libwebthree/IPFS.h>
using namespace std;
using namespace dev;

void help()
{
	cout
		<< "Usage rlp <mode> [OPTIONS]" << endl
		<< "Modes:" << endl
		<< "    convert <base-from> <base-to>  Take some input in one base and encode into another." << endl
		<< endl
		<< "Bases:" << endl
		<< "    sha3       Keccak (SHA3 winning) 256-bit hash (output only)." << endl
		<< "    sha256     SHA2-256 hash (output only)." << endl
		<< "    ripemd160  RIPEMD-160 hash (output only)." << endl
		<< "    hex        Hex-encoded." << endl
		<< "    base58     Base-58 encoded (IPFS/Bitcoin alphabet)." << endl
		<< "    base58alt  Base-58 encoded (Flickr alphabet)." << endl
		<< "    base64     Base-64 encoded." << endl
		<< "    bin        Binary/raw." << endl
		<< endl
		<< "General options:" << endl
		<< "    -h,--help  Print this help message and exit." << endl
		<< "    -V,--version  Show the version and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "exp version " << dev::Version << endl;
	exit(0);
}

enum class Encoding {
	Auto,
	Keccak,
	SHA2_256,
	RIPEMD160,
	Hex,
	Base58,
	Base58Alt,
	Base64,
	Binary,
};

Encoding toEncoding(std::string const& _s)
{
	if (_s == "auto")
		return Encoding::Auto;
	if (_s == "keccak" || _s == "sha3" || _s == "sha3-256")
		return Encoding::Keccak;
	if (_s == "sha256" || _s == "sha2-256")
		return Encoding::SHA2_256;
	if (_s == "ripemd160")
		return Encoding::RIPEMD160;
	if (_s == "hex" || _s == "16" || _s == "base16")
		return Encoding::Hex;
	if (_s == "bin" || _s == "binary" || _s == "256" || _s == "base256")
		return Encoding::Binary;
	if (_s == "58" || _s == "base58")
		return Encoding::Base58;
	if (_s == "58alt" || _s == "base58alt")
		return Encoding::Base58Alt;
	if (_s == "64" || _s == "base64")
		return Encoding::Base64;
	throw invalid_argument("Unknown encoding type");
}

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

void putOut(bytes const& _out, Encoding _encoding)
{
	switch (_encoding)
	{
	case Encoding::Hex: case Encoding::Auto:
		cout << toHex(_out) << endl;
		break;
	case Encoding::Base58:
		cout << toBase58(&_out) << endl;
		break;
	case Encoding::Base58Alt:
		cout << toBase58(&_out, AlphabetFlickr) << endl;
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
	case Encoding::SHA2_256:
		cout << sha256(&_out).hex() << endl;
		break;
	case Encoding::RIPEMD160:
		cout << ripemd160(&_out).hex() << endl;
		break;
	}
}

enum class Mode
{
	Convert,
	Upload
};

int main(int argc, char** argv)
{
	Mode mode = Mode::Convert;
	Encoding encodingFrom = Encoding::Auto;
	Encoding encodingTo = Encoding::Auto;
	string inputFile;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "convert" && i + 2 < argc)
		{
			mode = Mode::Convert;
			encodingFrom = toEncoding(argv[++i]);
			encodingTo = toEncoding(argv[++i]);
		}
		else if (arg == "upload")
			mode = Mode::Upload;
		else if (inputFile.empty())
			inputFile = arg;
		else
		{
			cerr << "Unexpected/unknown argument given: " << arg << endl;
			exit(-1);
		}
	}

	bytes in;
	if (inputFile == "--" || inputFile.empty())
		for (int i = cin.get(); i != -1; i = cin.get())
			in.push_back((byte)i);
	else if (boost::filesystem::is_regular_file(inputFile))
		in = contents(inputFile);
	else
		in = asBytes(inputFile);

	if (mode == Mode::Upload)
		cout << IPFS().putBlockForSHA256(&in).hex() << endl;
	else if (mode == Mode::Convert)
	{
		if (encodingFrom == Encoding::Auto)
		{
			encodingFrom = Encoding::Hex;
			for (char b: in)
				if (b != '\n' && b != ' ' && b != '\t')
				{
					if (encodingFrom == Encoding::Hex && (b < '0' || b > '9' ) && (b < 'a' || b > 'f' ) && (b < 'A' || b > 'F' ))
						encodingFrom = Encoding::Base64;
					if (encodingFrom == Encoding::Base64 && (b < '0' || b > '9' ) && (b < 'a' || b > 'z' ) && (b < 'A' || b > 'Z' ) && b != '+' && b != '/')
					{
						encodingFrom = Encoding::Binary;
						break;
					}
				}
		}

		bytes data;
		switch (encodingFrom)
		{
		case Encoding::Hex:
		case Encoding::Base58:
		case Encoding::Base58Alt:
		case Encoding::Base64:
		{
			string s = asString(in);
			boost::algorithm::replace_all(s, " ", "");
			boost::algorithm::replace_all(s, "\n", "");
			boost::algorithm::replace_all(s, "\t", "");
			switch (encodingFrom)
			{
			case Encoding::Hex:
				data = fromHex(s);
				break;
			case Encoding::Base58:
				data = fromBase58(s);
				break;
			case Encoding::Base58Alt:
				data = fromBase58(s, AlphabetFlickr);
				break;
			case Encoding::Base64:
				data = fromBase64(s);
				break;
			default: break;
			}
			break;
		}
		case Encoding::Binary:
			swap(data, in);
			break;
		case Encoding::SHA2_256:
			data = IPFS().getBlockForSHA256(h256(asString(in)));
			break;
		default:
			cerr << "Invalid input encoding." << endl;
			exit(-1);
		}

		putOut(data, encodingTo);
	}


	return 0;
}
