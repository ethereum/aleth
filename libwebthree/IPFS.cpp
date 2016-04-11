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
#include <libdevcore/Hash.h>
#include <libdevcore/Base58.h>
using namespace std;
using namespace dev;
#include "libexecstream/exec-stream.h"

static bytes exec(string const& _args)
{
	string output;
	try
	{
		exec_stream_t es("ipfs", _args);
		do
		{
			string s;
			getline(es.out(), s);
			output += s;
		} while(es.out().good());
	}
	catch (exception const &e)
	{
		throw IPFSCommunicationError(e.what());
	}
	return bytes(output.begin(), output.end());
}
static void exec(string const& _args, bytesConstRef _in)
{
	try
	{
		exec_stream_t es("ipfs", _args);
		es.in() << string(_in.begin(), _in.end());
	}
	catch (exception const &e)
	{
		throw IPFSCommunicationError(e.what());
	}
}

h256 IPFS::putBlockForSHA256(bytesConstRef _data)
{
	exec("block put", _data);
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
	return exec("block get " + toBase58(_multihash));
}
