// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/**
 * Determines the PoW algorithm.
 */

#include "EthashProofOfWork.h"
#include "Ethash.h"
using namespace std;
using namespace chrono;
using namespace dev;
using namespace eth;

const unsigned EthashProofOfWork::defaultLocalWorkSize = 64;
const unsigned EthashProofOfWork::defaultGlobalWorkSizeMultiplier = 4096; // * CL_DEFAULT_LOCAL_WORK_SIZE
const unsigned EthashProofOfWork::defaultMSPerBatch = 0;
const EthashProofOfWork::WorkPackage EthashProofOfWork::NullWorkPackage = EthashProofOfWork::WorkPackage();

EthashProofOfWork::WorkPackage::WorkPackage(BlockHeader const& _bh):
	boundary(Ethash::boundary(_bh)),
	seedHash(Ethash::seedHash(_bh)),
	m_headerHash(_bh.hash(WithoutSeal))
{}

EthashProofOfWork::WorkPackage::WorkPackage(WorkPackage const& _other):
	boundary(_other.boundary),
	seedHash(_other.seedHash),
	m_headerHash(_other.headerHash())
{}

EthashProofOfWork::WorkPackage& EthashProofOfWork::WorkPackage::operator=(EthashProofOfWork::WorkPackage const& _other)
{
	if (this == &_other)
		return *this;
	boundary = _other.boundary;
	seedHash = _other.seedHash;
	h256 headerHash = _other.headerHash();
	{
		Guard l(m_headerHashLock);
		m_headerHash = std::move(headerHash);
	}
	return *this;
}
