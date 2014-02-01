/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Main test functions.
 */

// TODO: utilise the shared testdata.

int trieTest();
int rlpTest();
int daggerTest();
int cryptoTest();
int stateTest();
int hexPrefixTest();
int peerTest(int argc, char** argv);

#include <BlockInfo.h>
using namespace eth;

int main(int argc, char** argv)
{
	RLPStream s;
	BlockInfo::genesis().fillStream(s, false);
	std::cout << RLP(s.out()) << std::endl;
	std::cout << asHex(s.out()) << std::endl;
	std::cout << sha3(s.out()) << std::endl;

//	hexPrefixTest();
//	rlpTest();
	trieTest();
//	daggerTest();
//	cryptoTest();
//	stateTest();
	peerTest(argc, argv);
	return 0;
}

