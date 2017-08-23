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
/** @file whisperMessage.cpp
* @author Vladislav Gluhovsky <vlad@ethdev.com>
* @date May 2015
*/

#include <boost/test/unit_test.hpp>
#include <libwhisper/Message.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::shh;

Topics createRandomTopics(unsigned int i)
{
	Topics ret;
	h256 t(i);

	for (int j = 0; j < 8; ++j)
	{
		t = sha3(t);
		ret.push_back(t);
	}

	return ret;
}

bytes createRandomPayload(unsigned int i)
{
	bytes ret;
	srand(i);
	int const sz = rand() % 1024;
	for (int j = 0; j < sz; ++j)
		ret.push_back(rand() % 256);

	return ret;
}

void comparePayloads(Message const& m1, Message const& m2)
{
	bytes const& p1 = m1.payload();
	bytes const& p2 = m2.payload();
	BOOST_REQUIRE_EQUAL(p1.size(), p2.size());

	for (size_t i = 0; i < p1.size(); ++i)
		BOOST_REQUIRE_EQUAL(p1[i], p2[i]);
}

void sealAndOpenSingleMessage(unsigned int i)
{
	Secret zero;
	Topics topics = createRandomTopics(i);
	bytes const payload = createRandomPayload(i);
	Message m1(payload);
	Envelope e = m1.seal(zero, topics, 1, 1);

	for (auto const& t: topics)
	{
		Topics singleTopic;
		singleTopic.push_back(t);
		Message m2(e, singleTopic, zero);
		comparePayloads(m1, m2);
	}
}

BOOST_FIXTURE_TEST_SUITE(whisperMessage, TestOutputHelper)

//
// Disabled tests as they are unstable and tend to stall the test suite.
//

//(seal)
//{
//	VerbosityHolder setTemporaryLevel(10);

//	cnote << "Testing Envelope encryption...";

//	for (unsigned int i = 1; i < 10; ++i)
//		sealAndOpenSingleMessage(i);
//}

//(work)
//{
//	VerbosityHolder setTemporaryLevel(10);
//	cnote << "Testing proof of work...";

//	Secret zero;
//	unsigned r = 0xC0DEFEED;

//	for (int i = 0; i < 20; ++i)
//	{
//		Topics topics = createRandomTopics(++r);
//		bytes const payload = createRandomPayload(++r);
//		Message m(payload);
//		Envelope e = m.seal(zero, topics, 1, 50);
//		unsigned x = e.workProved();
//		//cnote << x;
//		BOOST_REQUIRE(x > 4);
//	}
//}

BOOST_AUTO_TEST_SUITE_END()
