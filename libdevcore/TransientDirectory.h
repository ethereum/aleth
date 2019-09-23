// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <string>

namespace dev
{

/**
 * @brief temporary directory implementation
 * It creates temporary directory in the given path. On dealloc it removes the directory
 * @throws if the given path already exists, throws an exception
 */
class TransientDirectory
{
public:
	TransientDirectory();
	TransientDirectory(std::string const& _path);
	~TransientDirectory();

	std::string const& path() const { return m_path; }

private:
	std::string m_path;
};

}
