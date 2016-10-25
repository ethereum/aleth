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
#include <libdevcore/MemoryDB.h>
#include <libdevcore/TrieDB.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;

void help()
{
	cout
		<< "Usage bench <mode> [OPTIONS]" << endl
		<< "Modes:" << endl
		<< "    trie  Trie benchmarks." << endl
		<< endl
		<< "General options:" << endl
		<< "    -h,--help  Print this help message and exit." << endl
		<< "    -V,--version  Show the version and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "bench part of dev suite version " << dev::Version << endl;
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
	Trie,
	SHA3
};

enum class Alphabet
{
	Low, Mid, All
};

class StandardMap
{
public:
	StandardMap() = default;
	StandardMap(Alphabet _alphabet, unsigned _count, unsigned _minKey, unsigned _diffKey): m_alphabet(_alphabet), m_count(_count), m_minKey(_minKey), m_diffKey(_diffKey) {}

	vector<pair<bytes, bytes>> make() const
	{
		string const c_low = "abcdef";
		string const c_mid = "@QWERTYUIOPASDFGHJKLZXCVBNM[/]^_";

		h256 seed;
		vector<pair<bytes, bytes>> input;
		for (unsigned j = 0; j < m_count; ++j)
		{
			bytes k = m_alphabet == Alphabet::All ? randomBytes(m_minKey, m_diffKey, &seed) : randomWord(m_alphabet == Alphabet::Low ? c_low : c_mid, m_minKey, m_diffKey, &seed);
			bytes v = randomValue(&seed);
			input.push_back(make_pair(k, v));
		}
		return input;
	}

private:
	Alphabet m_alphabet;
	unsigned m_count;
	unsigned m_minKey;
	unsigned m_diffKey;

	static bytes randomWord(bytesConstRef _alphabet, unsigned _min, unsigned _diff, h256* _seed)
	{
		assert(_min + _diff < 33);
		*_seed = sha3(*_seed);
		unsigned l = _min + (*_seed)[31] % (_diff + 1);
		bytes ret;
		for (unsigned i = 0; i < l; ++i)
			ret.push_back(_alphabet[(*_seed)[i] % _alphabet.size()]);
		return ret;
	}

	static bytes randomBytes(unsigned _min, unsigned _diff, h256* _seed)
	{
		assert(_min + _diff < 33);
		*_seed = sha3(*_seed);
		unsigned l = _min + (*_seed)[31] % (_diff + 1);
		return _seed->ref().cropped(0, l).toBytes();
	}

	static bytes randomValue(h256* _seed)
	{
		*_seed = sha3(*_seed);
		if ((*_seed)[0] % 2)
			return bytes(1, (*_seed)[31]);
		else
			return _seed->asBytes();
	}
};

int main(int argc, char** argv)
{
	setDefaultOrCLocale();
	Mode mode = Mode::Trie;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "trie")
			mode = Mode::Trie;
		else if (arg == "sha3")
			mode = Mode::SHA3;
		else if (arg == "-V" || arg == "--version")
			version();
	}

	if (mode == Mode::Trie)
	{
		unordered_map<string, StandardMap> maps =
		{
			{ "six-low", StandardMap(Alphabet::Low, 1000, 6, 0) },
			{ "six-mid", StandardMap(Alphabet::Mid, 1000, 6, 0) },
			{ "six-all", StandardMap(Alphabet::All, 1000, 6, 0) },
			{ "mix-mid", StandardMap(Alphabet::Mid, 1000, 1, 5) },
		};
		unsigned trials = 50;
		for (auto const& sm: maps)
		{
			MemoryDB mdb;
			GenericTrieDB<MemoryDB> t(&mdb);
			t.init();

			auto map = sm.second.make();
			Timer timer;
			for (unsigned i = 0; i < trials; ++i)
				for (auto const& i: map)
					t.insert(&i.first, &i.second);
			auto e = timer.elapsed() / trials;

			cout << sm.first << ": " << e * 1000000 << " us, root=" << t.root() << endl;
		}
	}
	else if (mode == Mode::SHA3)
	{
		unsigned trials = 50;
		Timer t;
		for (unsigned trial = 0; trial < trials; ++trial)
		{
			h256 s;
			for (unsigned i = 0; i < 1000; ++i)
				s = sha3(s);
		}
		cout << "sha3 x 1000: " << t.elapsed() / trials * 1000000 << "us " << endl;
	}

	return 0;
}
