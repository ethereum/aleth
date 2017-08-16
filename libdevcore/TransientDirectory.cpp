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

#include <thread>
#include <boost/filesystem.hpp>
#include "Exceptions.h"
#include "TransientDirectory.h"
#include "CommonIO.h"
#include "Log.h"

namespace fs = boost::filesystem;

namespace dev
{

TransientDirectory::TransientDirectory()
{
	fs::path root{"/dev/shm"};
	if (!fs::is_directory(root))
		root = fs::temp_directory_path();

	m_path = (root / ("eth_" + toString(FixedHash<4>::random()))).string();

	// we never ever want to delete a directory (including all its contents) that we did not create ourselves.
	if (fs::exists(m_path))
		BOOST_THROW_EXCEPTION(FileError());

	if (!fs::create_directories(m_path))
		BOOST_THROW_EXCEPTION(FileError());
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
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	ec.clear();
	fs::remove_all(m_path, ec);
	if (!ec)
	{
		cwarn << "Failed to delete directory '" << m_path << "': " << ec.message();
	}
}

}