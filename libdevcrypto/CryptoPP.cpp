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
/** @file CryptoPP.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * CryptoPP wrappers
 */

#include "CryptoPP.h"

using namespace dev;
using namespace dev::crypto;
using namespace pp;
using namespace CryptoPP;

void pp::exportDL_PublicKey_EC(CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> const& _k, Public& _p) {
	ECP::Point q(_k.GetPublicElement());
	q.x.Encode(_p.data(), 32);
	q.y.Encode(&_p.data()[32], 32);
}

void pp::exportDL_PrivateKey_EC(CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> const& _k, Secret& _s) {
	_k.GetPrivateExponent().Encode(_s.data(), 32);
}

ECP::Point pp::PointFromPublic(Public const& _p)
{
	Integer x(&_p.data()[0], 32);
	Integer y(&_p.data()[32], 32);
	return std::move(ECP::Point(x,y));
}

Integer pp::ExponentFromSecret(Secret const& _s)
{
	return std::move(Integer(_s.data(), 32));
}





pp::ECKeyPair::ECKeyPair():
m_decryptor(pp::PRNG(), pp::secp256k1())
{
}

Secret pp::ECKeyPair::secret()
{
	Secret s;
	exportDL_PrivateKey_EC(m_decryptor.AccessKey(), s);
	return std::move(s);
}