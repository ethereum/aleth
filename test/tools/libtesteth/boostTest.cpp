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
/** @file
 * Stub for generating main boost.test module.
 * Original code taken from boost sources.
 */


#define BOOST_TEST_MODULE EthereumTests
#define BOOST_TEST_NO_MAIN
#include <boost/test/included/unit_test.hpp>

#include <clocale>
#include <cstdlib>
#include <iostream>
#include <test/tools/libtesteth/TestHelper.h>

using namespace boost::unit_test;

std::vector<char*> parameters;
static std::ostringstream strCout;
std::streambuf* oldCoutStreamBuf;
std::streambuf* oldCerrStreamBuf;

void createRandomTestWrapper()
{
	//restore output for creating test
	std::cout.rdbuf(oldCoutStreamBuf);
	std::cerr.rdbuf(oldCerrStreamBuf);

	//For no reason BOOST tend to remove valuable arg -t "TestSuiteName"
	//And can't hadle large input stream of data
	//so the actual test suite and raw test input is read into Options
	if (dev::test::createRandomTest())
		throw framework::internal_error("Create Random Test Error!");

	exit(0);
}

static std::atomic_bool stopTravisOut;
void travisOut()
{
	int tickCounter = 0;
	while (!stopTravisOut)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		++tickCounter;
		if (tickCounter % 10 == 0)
			std::cout << ".\n" << std::flush;  // Output dot every 10s.
	}
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

//Custom Boost Unit Test Main
int main( int argc, char* argv[] )
{
	setDefaultOrCLocale();
	try
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
			// FIXME:
			test_suite* ts1 = BOOST_TEST_SUITE("RandomTestCreationSuite");
			ts1->add(BOOST_TEST_CASE(&createRandomTestWrapper));
			framework::master_test_suite().add(ts1);
		}
	}
	catch (dev::test::Options::InvalidOption const& e)
	{
		std::cerr << e.what() << "\n";
		exit(1);
	}

	for (int i = 0; i < argc; i++)
		parameters.push_back(argv[i]);

	stopTravisOut = false;
	std::thread outputThread(travisOut);
	auto fakeInit = [](int, char*[]) -> boost::unit_test::test_suite* { return nullptr; };
	int result = unit_test_main(fakeInit, argc, argv);
	stopTravisOut = true;
	outputThread.join();
	dev::test::TestOutputHelper::printTestExecStats();
	return result;
}
