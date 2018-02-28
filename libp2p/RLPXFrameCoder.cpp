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
/** @file RLPXFrameCoder.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2015
 */

#include "RLPXFrameCoder.h"
#include <cryptopp/aes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/modes.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/SHA3.h>
#include "RLPxHandshake.h"
#include "RLPXPacket.h"

static_assert(CRYPTOPP_VERSION == 565, "Wrong Crypto++ version");

using namespace std;
using namespace dev;
using namespace dev::p2p;

RLPXFrameInfo::RLPXFrameInfo(bytesConstRef _header):
	length(to_integer(_header[0]) * 256 + to_integer(_header[1]) * 256 + to_integer(_header[2])),
	padding((16 - (length % 16)) % 16),
	data(_header.cropped(3).toBytes()),
	header(RLP(data, RLP::ThrowOnFail | RLP::FailIfTooSmall)),
	protocolId(header[0].toInt<uint16_t>()),
	multiFrame(header.itemCount() > 1),
	sequenceId(multiFrame ? header[1].toInt<uint16_t>() : 0),
	totalLength(header.itemCount() == 3 ? header[2].toInt<uint32_t>() : 0)
{}

namespace dev
{
namespace p2p
{
class RLPXFrameCoderImpl
{
public:
	/// Update state of _mac.
	void updateMAC(CryptoPP::Keccak_256& _mac, bytesConstRef _seed = {});

	CryptoPP::SecByteBlock frameEncKey;						///< Key for m_frameEnc
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption frameEnc;	///< Encoder for egress plaintext.

	CryptoPP::SecByteBlock frameDecKey;						///< Key for m_frameDec
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption frameDec;	///< Decoder for egress plaintext.

	CryptoPP::SecByteBlock macEncKey;						/// Key for m_macEnd
	CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption macEnc;	/// One-way coder used by updateMAC for ingress and egress MAC updates.

	CryptoPP::Keccak_256 egressMac;		///< State of MAC for egress ciphertext.
	CryptoPP::Keccak_256 ingressMac;	///< State of MAC for ingress ciphertext.

private:
	Mutex x_macEnc;  ///< Mutex.
};
}
}

RLPXFrameCoder::~RLPXFrameCoder()
{}

RLPXFrameCoder::RLPXFrameCoder(RLPXHandshake const& _init):
	m_impl(new RLPXFrameCoderImpl)
{
	setup(_init.m_originated, _init.m_ecdheRemote, _init.m_remoteNonce, _init.m_ecdheLocal, _init.m_nonce, &_init.m_ackCipher, &_init.m_authCipher);
}

RLPXFrameCoder::RLPXFrameCoder(bool _originated, h512 const& _remoteEphemeral, h256 const& _remoteNonce, KeyPair const& _ecdheLocal, h256 const& _nonce, bytesConstRef _ackCipher, bytesConstRef _authCipher):
	m_impl(new RLPXFrameCoderImpl)
{
	setup(_originated, _remoteEphemeral, _remoteNonce, _ecdheLocal, _nonce, _ackCipher, _authCipher);
}

void RLPXFrameCoder::setup(bool _originated, h512 const& _remoteEphemeral, h256 const& _remoteNonce, KeyPair const& _ecdheLocal, h256 const& _nonce, bytesConstRef _ackCipher, bytesConstRef _authCipher)
{
	bytes keyMaterialBytes(64);
	bytesRef keyMaterial(&keyMaterialBytes);

	// shared-secret = sha3(ecdhe-shared-secret || sha3(nonce || initiator-nonce))
	Secret ephemeralShared;

	// Try agree ECDHE. This can fail due to invalid remote public key. In this
	// case throw an exception to be caught RLPXHandshake::transition().
	if (!crypto::ecdh::agree(_ecdheLocal.secret(), _remoteEphemeral, ephemeralShared))
		BOOST_THROW_EXCEPTION(ECDHEError{});

	ephemeralShared.ref().copyTo(keyMaterial.cropped(0, h256::size));
	h512 nonceMaterial;
	h256 const& leftNonce = _originated ? _remoteNonce : _nonce;
	h256 const& rightNonce = _originated ? _nonce : _remoteNonce;
	leftNonce.ref().copyTo(nonceMaterial.ref().cropped(0, h256::size));
	rightNonce.ref().copyTo(nonceMaterial.ref().cropped(h256::size, h256::size));
	auto outRef(keyMaterial.cropped(h256::size, h256::size));
	sha3(nonceMaterial.ref(), outRef); // output h(nonces)
	
	sha3(keyMaterial, outRef); // output shared-secret
	// token: sha3(outRef, bytesRef(&token)); -> m_host (to be saved to disk)
	
	// aes-secret = sha3(ecdhe-shared-secret || shared-secret)
	sha3(keyMaterial, outRef); // output aes-secret
	assert(m_impl->frameEncKey.empty() && "ECDHE aggreed before!");
	m_impl->frameEncKey.resize(h256::size);
	memcpy(m_impl->frameEncKey.data(), outRef.data(), h256::size);
	m_impl->frameDecKey.resize(h256::size);
	memcpy(m_impl->frameDecKey.data(), outRef.data(), h256::size);
	h128 iv;
	m_impl->frameEnc.SetKeyWithIV(m_impl->frameEncKey, h256::size, reinterpret_cast<const unsigned char *>(iv.data()));
	m_impl->frameDec.SetKeyWithIV(m_impl->frameDecKey, h256::size, reinterpret_cast<const unsigned char *>(iv.data()));

	// mac-secret = sha3(ecdhe-shared-secret || aes-secret)
	sha3(keyMaterial, outRef); // output mac-secret
	m_impl->macEncKey.resize(h256::size);
	memcpy(m_impl->macEncKey.data(), outRef.data(), h256::size);
	m_impl->macEnc.SetKey(m_impl->macEncKey, h256::size);

	// Initiator egress-mac: sha3(mac-secret^recipient-nonce || auth-sent-init)
	//           ingress-mac: sha3(mac-secret^initiator-nonce || auth-recvd-ack)
	// Recipient egress-mac: sha3(mac-secret^initiator-nonce || auth-sent-ack)
	//           ingress-mac: sha3(mac-secret^recipient-nonce || auth-recvd-init)
 
	(*(h256*)outRef.data() ^ _remoteNonce).ref().copyTo(keyMaterial);
	bytesConstRef egressCipher = _originated ? _authCipher : _ackCipher;
	keyMaterialBytes.resize(h256::size + egressCipher.size());
	keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
	egressCipher.copyTo(keyMaterial.cropped(h256::size, egressCipher.size()));
	m_impl->egressMac.Update(reinterpret_cast<const unsigned char *>(keyMaterial.data()), keyMaterial.size());

	// recover mac-secret by re-xoring remoteNonce
	(*(h256*)keyMaterial.data() ^ _remoteNonce ^ _nonce).ref().copyTo(keyMaterial);
	bytesConstRef ingressCipher = _originated ? _ackCipher : _authCipher;
	keyMaterialBytes.resize(h256::size + ingressCipher.size());
	keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
	ingressCipher.copyTo(keyMaterial.cropped(h256::size, ingressCipher.size()));
	m_impl->ingressMac.Update(reinterpret_cast<const unsigned char *>(keyMaterial.data()), keyMaterial.size());
}

void RLPXFrameCoder::writeFrame(uint16_t _protocolType, bytesConstRef _payload, bytes& o_bytes)
{
	RLPStream header;
	uint32_t len = (uint32_t)_payload.size();
	header.appendRaw(bytes({byte((len >> 16) & 0xff), byte((len >> 8) & 0xff), byte(len & 0xff)}));
	header.appendList(1) << _protocolType;
	writeFrame(header, _payload, o_bytes);
}

void RLPXFrameCoder::writeFrame(uint16_t _protocolType, uint16_t _seqId, bytesConstRef _payload, bytes& o_bytes)
{
	RLPStream header;
	uint32_t len = (uint32_t)_payload.size();
	header.appendRaw(bytes({byte((len >> 16) & 0xff), byte((len >> 8) & 0xff), byte(len & 0xff)}));
	header.appendList(2) << _protocolType << _seqId;
	writeFrame(header, _payload, o_bytes);
}

void RLPXFrameCoder::writeFrame(uint16_t _protocolType, uint16_t _seqId, uint32_t _totalSize, bytesConstRef _payload, bytes& o_bytes)
{
	RLPStream header;
	uint32_t len = (uint32_t)_payload.size();
	header.appendRaw(bytes({byte((len >> 16) & 0xff), byte((len >> 8) & 0xff), byte(len & 0xff)}));
	header.appendList(3) << _protocolType << _seqId << _totalSize;
	writeFrame(header, _payload, o_bytes);
}

void RLPXFrameCoder::writeFrame(RLPStream const& _header, bytesConstRef _payload, bytes& o_bytes)
{
	// TODO: SECURITY check header values && header <= 16 bytes
	bytes headerWithMac(h256::size);
	bytesConstRef(&_header.out()).copyTo(bytesRef(&headerWithMac));
	m_impl->frameEnc.ProcessData(reinterpret_cast<unsigned char *>(headerWithMac.data()), reinterpret_cast<const unsigned char *>(headerWithMac.data()), 16);
	updateEgressMACWithHeader(bytesConstRef(&headerWithMac).cropped(0, 16));
	egressDigest().ref().copyTo(bytesRef(&headerWithMac).cropped(h128::size,h128::size));

	auto padding = (16 - (_payload.size() % 16)) % 16;
	o_bytes.swap(headerWithMac);
	o_bytes.resize(32 + _payload.size() + padding + h128::size);
	bytesRef packetRef(o_bytes.data() + 32, _payload.size());
	m_impl->frameEnc.ProcessData(reinterpret_cast<unsigned char *>(packetRef.data()), reinterpret_cast<const unsigned char *>(_payload.data()), _payload.size());
	bytesRef paddingRef(o_bytes.data() + 32 + _payload.size(), padding);
	if (padding)
		m_impl->frameEnc.ProcessData(reinterpret_cast<unsigned char *>(paddingRef.data()), reinterpret_cast<const unsigned char *>(paddingRef.data()), padding);
	bytesRef packetWithPaddingRef(o_bytes.data() + 32, _payload.size() + padding);
	updateEgressMACWithFrame(packetWithPaddingRef);
	bytesRef macRef(o_bytes.data() + 32 + _payload.size() + padding, h128::size);
	egressDigest().ref().copyTo(macRef);
}

void RLPXFrameCoder::writeSingleFramePacket(bytesConstRef _packet, bytes& o_bytes)
{
	RLPStream header;
	uint32_t len = (uint32_t)_packet.size();
	header.appendRaw(bytes({byte((len >> 16) & 0xff), byte((len >> 8) & 0xff), byte(len & 0xff)}));
	header.appendRaw(bytes({static_cast<byte>(0xc2),static_cast<byte>(0x80),static_cast<byte>(0x80)}));
	writeFrame(header, _packet, o_bytes);
}

bool RLPXFrameCoder::authAndDecryptHeader(bytesRef io)
{
	asserts(io.size() == h256::size);
	updateIngressMACWithHeader(io);
	bytesConstRef macRef = io.cropped(h128::size, h128::size);
	h128 expected = ingressDigest();
	if (*(h128*)macRef.data() != expected)
		return false;
	m_impl->frameDec.ProcessData(reinterpret_cast<unsigned char *>(io.data()), reinterpret_cast<const unsigned char *>(io.data()), h128::size);
	return true;
}

bool RLPXFrameCoder::authAndDecryptFrame(bytesRef io)
{
	bytesRef cipherText(io.cropped(0, io.size() - h128::size));
	updateIngressMACWithFrame(cipherText);
	bytesConstRef frameMac(io.data() + io.size() - h128::size, h128::size);
	if (*(h128*)frameMac.data() != ingressDigest())
		return false;
	m_impl->frameDec.ProcessData(reinterpret_cast<unsigned char *>(io.data()), reinterpret_cast<const unsigned char *>(io.data()), io.size() - h128::size);
	return true;
}

h128 RLPXFrameCoder::egressDigest()
{
	CryptoPP::Keccak_256 h(m_impl->egressMac);
	h128 digest;
	h.TruncatedFinal(reinterpret_cast<unsigned char *>(digest.data()), h128::size);
	return digest;
}

h128 RLPXFrameCoder::ingressDigest()
{
	CryptoPP::Keccak_256 h(m_impl->ingressMac);
	h128 digest;
	h.TruncatedFinal(reinterpret_cast<unsigned char *>(digest.data()), h128::size);
	return digest;
}

void RLPXFrameCoder::updateEgressMACWithHeader(bytesConstRef _headerCipher)
{
	m_impl->updateMAC(m_impl->egressMac, _headerCipher.cropped(0, 16));
}

void RLPXFrameCoder::updateEgressMACWithFrame(bytesConstRef _cipher)
{
	m_impl->egressMac.Update(reinterpret_cast<const unsigned char *>(_cipher.data()), _cipher.size());
	m_impl->updateMAC(m_impl->egressMac);
}

void RLPXFrameCoder::updateIngressMACWithHeader(bytesConstRef _headerCipher)
{
	m_impl->updateMAC(m_impl->ingressMac, _headerCipher.cropped(0, 16));
}

void RLPXFrameCoder::updateIngressMACWithFrame(bytesConstRef _cipher)
{
	m_impl->ingressMac.Update(reinterpret_cast<const unsigned char *>(_cipher.data()), _cipher.size());
	m_impl->updateMAC(m_impl->ingressMac);
}

void RLPXFrameCoderImpl::updateMAC(CryptoPP::Keccak_256& _mac, bytesConstRef _seed)
{
	if (_seed.size() && _seed.size() != h128::size)
		asserts(false);

	CryptoPP::Keccak_256 prevDigest(_mac);
	h128 encDigest(h128::size);
	prevDigest.TruncatedFinal(reinterpret_cast<unsigned char *>(encDigest.data()), h128::size);
	h128 prevDigestOut = encDigest;

	{
		Guard l(x_macEnc);
		macEnc.ProcessData(reinterpret_cast<unsigned char *>(encDigest.data()), reinterpret_cast<const unsigned char *>(encDigest.data()), 16);
	}
	if (_seed.size())
		encDigest ^= *(h128*)_seed.data();
	else
		encDigest ^= *(h128*)prevDigestOut.data();

	// update mac for final digest
	_mac.Update(reinterpret_cast<const unsigned char *>(encDigest.data()), h128::size);
}
