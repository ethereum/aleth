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
/** @file InterfaceStub.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once

#include "Interface.h"

namespace dev {

namespace eth {

class InterfaceStub: public dev::eth::Interface
{
public:
	InterfaceStub() {}
	virtual ~InterfaceStub() {}
	
	/// Submits the given message-call transaction.
	virtual void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo) override;

	/// Submits a new contract-creation transaction.
	/// @returns the new contract's address (assuming it all goes through).
	virtual Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas = 10000, u256 _gasPrice = 10 * szabo) override;
	
	/// Injects the RLP-encoded transaction given by the _rlp into the transaction queue directly.
	virtual void inject(bytesConstRef _rlp) override;
	
	/// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() override;
	
	/// Makes the given call. Nothing is recorded into the state.
	virtual bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo, int _blockNumber = 0) override;
	
	/// Makes the given call. Nothing is recorded into the state. This cheats by creating a null address and endowing it with a lot of ETH.
	virtual bytes call(Address _dest, bytes const& _data = bytes(), u256 _gas = 125000, u256 _value = 0, u256 _gasPrice = 1 * ether);
	
	using Interface::balanceAt;
	using Interface::countAt;
	using Interface::stateAt;
	using Interface::codeAt;
	using Interface::storageAt;

	u256 balanceAt(Address _a, int _block) const override;
	u256 countAt(Address _a, int _block) const override;
	u256 stateAt(Address _a, u256 _l, int _block) const override;
	bytes codeAt(Address _a, int _block) const override;
	std::map<u256, u256> storageAt(Address _a, int _block) const override;

	h256 hashFromNumber(unsigned _number) const override;
	eth::BlockInfo blockInfo(h256 _hash) const override;
	eth::BlockDetails blockDetails(h256 _hash) const override;
	eth::Transaction transaction(h256 _transactionHash) const override;
	eth::Transaction transaction(h256 _blockHash, unsigned _i) const override;
	eth::Transactions transactions(h256 _blockHash) const override;
	eth::TransactionHashes transactionHashes(h256 _blockHash) const override;
	eth::BlockInfo uncle(h256 _blockHash, unsigned _i) const override;
	unsigned transactionCount(h256 _blockHash) const override;
	unsigned uncleCount(h256 _blockHash) const override;
	unsigned number() const override;
	eth::Transactions pending() const override;

	using Interface::addresses;
	Addresses addresses(int _block) const override;
	u256 gasLimitRemaining() const override;

	/// Set coinbase address.
	void setAddress(Address _us) override;
	/// Get the coinbase address.`
	Address address() const override;

virtual BlockChain const& bc() const = 0;	
virtual State asOf(int _h) const = 0;
virtual State preMine() const = 0;
virtual State postMine() const = 0;

protected:
	
	TransactionQueue m_tq;					///< Maintains a list of incoming transactions not yet in a block on the blockchain.

private:

};

}}
