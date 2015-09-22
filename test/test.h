
#pragma once
#include <boost/test/unit_test.hpp>

namespace dev
{
namespace test
{

	std::string getTestPath();

	class Options
	{
	public:
		bool performance = false;
		bool nonetwork = false;

		/// Get reference to options
		/// The first time used, options are parsed
		static Options const& get();

	private:
		Options();
		Options(Options const&) = delete;
	};

}}
