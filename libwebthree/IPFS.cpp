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
/** @file IPFS.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "IPFS.h"
#include <cstdio>
/*#include <boost/process.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>*/
#include <libdevcore/Hash.h>
#include <libdevcore/Base58.h>
using namespace std;
using namespace dev;
/*namespace bp = boost::process;
namespace bi = boost::iostreams;*/

#if WIN32
#define POPEN_R(X) _popen(X, "rb")
#define POPEN_W(X) _popen(X, "wb")
#define PCLOSE _pclose
#else
#define POPEN_R(X) popen(X, "r")
#define POPEN_W(X) popen(X, "w")
#define PCLOSE pclose
#endif

static bytes exec(std::string const& _cmd)
{
	FILE* pipe = POPEN_R(_cmd.c_str());
	if (!pipe)
		throw system_error();
	bytes ret;
	while (!feof(pipe))
	{
		ret.resize(ret.size() + 1024);
		auto read = fread(ret.data() + ret.size() - 1024, 1, 1024, pipe);
		ret.resize(ret.size() - 1024 + read);
	}
	PCLOSE(pipe);
	return ret;
}

static void exec(std::string const& _cmd, bytesConstRef _in)
{
	FILE* pipe = POPEN_W(_cmd.c_str());
	if (!pipe)
		throw system_error();
	fwrite(_in.data(), 1, _in.size(), pipe);
	PCLOSE(pipe);
}

h256 IPFS::putBlockForSHA256(bytesConstRef _data)
{
	exec("ipfs block put", _data);
	return sha256(_data);
}

bytes IPFS::putBlock(bytesConstRef _data)
{
	return sha256AsMultihash(putBlockForSHA256(_data));
}

bytes IPFS::getBlockForSHA256(h256 const& _sha256)
{
	auto b = sha256AsMultihash(_sha256);
	return getBlock(&b);
}

bytes IPFS::getBlock(bytesConstRef _multihash)
{
	return exec("ipfs block get " + toBase58(_multihash));
}
