// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libethcore/Common.h>
#include <libethcore/LogEntry.h>
#include <libdevcore/RLP.h>

#include <boost/variant/variant.hpp>
#include <array>

namespace dev
{
namespace eth
{

/// Transaction receipt, constructed either from RLP representation or from individual values.
/// Either a state root or a status code is contained.  m_hasStatusCode is true when it contains a status code.
/// Empty state root is not included into RLP-encoding.
class TransactionReceipt
{
public:
	TransactionReceipt(bytesConstRef _rlp);
	TransactionReceipt(h256 const& _root, u256 const& _gasUsed, LogEntries const& _log);
	TransactionReceipt(uint8_t _status, u256 const& _gasUsed, LogEntries const& _log);

	/// @returns true if the receipt has a status code.  Otherwise the receipt has a state root (pre-EIP658).
	bool hasStatusCode() const;
	/// @returns the state root.
	/// @throw TransactionReceiptVersionError when the receipt has a status code instead of a state root.
	h256 const& stateRoot() const;
	/// @returns the status code.
	/// @throw TransactionReceiptVersionError when the receipt has a state root instead of a status code.
	uint8_t statusCode() const;
	u256 const& cumulativeGasUsed() const { return m_gasUsed; }
	LogBloom const& bloom() const { return m_bloom; }
	LogEntries const& log() const { return m_log; }

	void streamRLP(RLPStream& _s) const;

	bytes rlp() const { RLPStream s; streamRLP(s); return s.out(); }

private:
	boost::variant<uint8_t,h256> m_statusCodeOrStateRoot;
	u256 m_gasUsed;
	LogBloom m_bloom;
	LogEntries m_log;
};

using TransactionReceipts = std::vector<TransactionReceipt>;

std::ostream& operator<<(std::ostream& _out, eth::TransactionReceipt const& _r);

class LocalisedTransactionReceipt: public TransactionReceipt
{
public:
	LocalisedTransactionReceipt(
		TransactionReceipt const& _t,
		h256 const& _hash,
		h256 const& _blockHash,
		BlockNumber _blockNumber,
		Address const& _from,
		Address const& _to, 
		unsigned _transactionIndex,
		u256 const& _gasUsed,
		Address const& _contractAddress = Address()
	):
		TransactionReceipt(_t),
		m_hash(_hash),
		m_blockHash(_blockHash),
		m_blockNumber(_blockNumber),
		m_from(_from),
		m_to(_to),
		m_transactionIndex(_transactionIndex),
		m_gasUsed(_gasUsed),
		m_contractAddress(_contractAddress)
	{
		LogEntries entries = log();
		for (unsigned i = 0; i < entries.size(); i++)
			m_localisedLogs.push_back(LocalisedLogEntry(
				entries[i],
				m_blockHash,
				m_blockNumber,
				m_hash,
				m_transactionIndex,
				i
			));
	}

	h256 const& hash() const { return m_hash; }
	h256 const& blockHash() const { return m_blockHash; }
	BlockNumber blockNumber() const { return m_blockNumber; }
	Address const& from() const { return m_from; }
	Address const& to() const { return m_to; }
	unsigned transactionIndex() const { return m_transactionIndex; }
	u256 const& gasUsed() const { return m_gasUsed; }
	Address const& contractAddress() const { return m_contractAddress; }
	LocalisedLogEntries const& localisedLogs() const { return m_localisedLogs; };

private:
	h256 m_hash;
	h256 m_blockHash;
	BlockNumber m_blockNumber;
	Address m_from;
	Address m_to;
	unsigned m_transactionIndex = 0;
	u256 m_gasUsed;
	Address m_contractAddress;
	LocalisedLogEntries m_localisedLogs;
};

}
}
