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
/** @file ECDHE.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * Elliptic curve Diffie-Hellman ephemeral key exchange
 */

#pragma once

#include "Common.h"

namespace dev
{
namespace crypto
{

/**
 * @brief Derive DH shared secret from EC keypairs.
 * As ephemeral keys are single-use, agreement is limited to a single occurrence.
 */
class ECDHE
{
public:
	/// Constructor (pass public key for ingress exchange).
	ECDHE(): m_ephemeral(KeyPair::create()) {};

	/// Public key sent to remote.
	Public pub() const { return m_ephemeral.pub(); }

	Secret secret() const { return m_ephemeral.secret(); }

private:
	KeyPair m_ephemeral;  ///< Ephemeral keypair; generated.
};

}
}

