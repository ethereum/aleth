// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include <thread>
#include <boost/filesystem.hpp>
#include "Exceptions.h"
#include "TransientDirectory.h"
#include "CommonIO.h"
#include "Log.h"
using namespace std;
using namespace dev;
namespace fs = boost::filesystem;

TransientDirectory::TransientDirectory():
	TransientDirectory((boost::filesystem::temp_directory_path() / "eth_transient" / toString(FixedHash<4>::random())).string())
{}

TransientDirectory::TransientDirectory(std::string const& _path):
	m_path(_path)
{
	// we never ever want to delete a directory (including all its contents) that we did not create ourselves.
	if (boost::filesystem::exists(m_path))
		BOOST_THROW_EXCEPTION(FileError());

	if (!fs::create_directories(m_path))
		BOOST_THROW_EXCEPTION(FileError());
	DEV_IGNORE_EXCEPTIONS(fs::permissions(m_path, fs::owner_all));
}

TransientDirectory::~TransientDirectory()
{
	boost::system::error_code ec;		
	fs::remove_all(m_path, ec);
	if (!ec)
		return;

	// In some cases, antivirus runnig on Windows will scan all the newly created directories.
	// As a consequence, directory is locked and can not be deleted immediately.
	// Retry after 10 milliseconds usually is successful.
	// This will help our tests run smoothly in such environment.
	this_thread::sleep_for(chrono::milliseconds(10));

	ec.clear();
	fs::remove_all(m_path, ec);
	if (!ec)
	{
		cwarn << "Failed to delete directory '" << m_path << "': " << ec.message();
	}
}
