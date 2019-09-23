// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/** 
 * AES
 * todo: use openssl
 */

#pragma once

#include "Common.h"

namespace dev
{

bytes aesDecrypt(bytesConstRef _cipher, std::string const& _password, unsigned _rounds = 2000, bytesConstRef _salt = bytesConstRef());

}
