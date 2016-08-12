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
/** @file boostTest.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * Stub for generating main boost.test module.
 * Original code taken from boost sources.
 */

#define BOOST_TEST_MODULE EthereumTests
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
//#define BOOST_DISABLE_WIN32 //disables SEH warning
#define BOOST_TEST_NO_MAIN

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4535) // calling _set_se_translator requires /EHa
#endif
#include <boost/test/included/unit_test.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#pragma GCC diagnostic pop

#include <stdlib.h>
#include <test/TestHelper.h>
#include <boost/version.hpp>

using namespace boost::unit_test;

std::vector<char*> parameters;
static std::ostringstream strCout;
std::streambuf* oldCoutStreamBuf;
std::streambuf* oldCerrStreamBuf;

//Custom Boost Initialization
test_suite* fake_init_func(int argc, char* argv[])
{
	//Required for boost. -nowarning
	(void) argc;
	(void) argv;
	return 0;
}

void createRandomTest()
{
	//restore output for creating test
	std::cout.rdbuf(oldCoutStreamBuf);
	std::cerr.rdbuf(oldCerrStreamBuf);

	//For no reason BOOST tend to remove valuable arg -t "TestSuiteName"
	//And can't hadle large input stream of data
	//so the actual test suite and raw test input is read into Options
	if (dev::test::createRandomTest(parameters))
		throw framework::internal_error("Create Random Test Error!");
	else
	{
		//disable post output so the test json would be clean
		if (dev::test::Options::get().rCheckTest.size() > 0)
			std::cout << "correct" << std::endl;
		exit(0);
	}
}

//Custom Boost Unit Test Main
int main( int argc, char* argv[] )
{
	//Initialize options
	dev::test::Options const& opt = dev::test::Options::get(argc, argv);

	if (opt.createRandomTest)
	{
		//disable initial output
		oldCoutStreamBuf = std::cout.rdbuf();
		oldCerrStreamBuf = std::cerr.rdbuf();
		std::cout.rdbuf(strCout.rdbuf());
		std::cerr.rdbuf(strCout.rdbuf());

		for (int i = 0; i < argc; i++)
		{
			std::string arg = std::string{argv[i]};

			//replace test suite to random tests
			if (arg == "-t" && i+1 < argc)
				argv[i+1] = (char*)std::string("RandomTestCreationSuite").c_str();

			//don't pass long raw test input to boost
			if (arg == "--checktest")
			{
				argc = i + 1;
				break;
			}
		}

		//add random tests suite
		test_suite* ts1 = BOOST_TEST_SUITE("RandomTestCreationSuite");
		ts1->add(BOOST_TEST_CASE(&createRandomTest));
		framework::master_test_suite().add(ts1);
	}

	for (int i = 0; i < argc; i++)
		parameters.push_back(argv[i]);

	return unit_test_main(fake_init_func, argc, argv);
}
