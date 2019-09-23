// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "UDP.h"
using namespace std;
using namespace dev;
using namespace dev::p2p;

h256 RLPXDatagramFace::sign(Secret const& _k)
{
    assert(packetType());
    
    RLPStream rlpxstream;
//	rlpxstream.appendRaw(toPublic(_k).asBytes()); // for mdc-based signature
    rlpxstream.appendRaw(bytes(1, packetType())); // prefix by 1 byte for type
    streamRLP(rlpxstream);
    bytes rlpxBytes(rlpxstream.out());
    
    bytesConstRef rlpx(&rlpxBytes);
    h256 sighash(dev::sha3(rlpx)); // H(type||data)
    Signature sig = dev::sign(_k, sighash); // S(H(type||data))
    
    data.resize(h256::size + Signature::size + rlpx.size());
    bytesRef rlpxHash(&data[0], h256::size);
    bytesRef rlpxSig(&data[h256::size], Signature::size);
    bytesRef rlpxPayload(&data[h256::size + Signature::size], rlpx.size());
    
    sig.ref().copyTo(rlpxSig);
    rlpx.copyTo(rlpxPayload);
    
    bytesConstRef signedRLPx(&data[h256::size], data.size() - h256::size);
    h256 hash(dev::sha3(signedRLPx));
    hash.ref().copyTo(rlpxHash);

    return hash;
}

Public RLPXDatagramFace::authenticate(bytesConstRef _sig, bytesConstRef _rlp)
{
    Signature const& sig = *(Signature const*)_sig.data();
    return dev::recover(sig, sha3(_rlp));
}

