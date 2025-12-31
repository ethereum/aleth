// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/vector_ref.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libevm/VMFace.h>
#include "Interface.h"
#include "Transaction.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#define ETH_ADDRESS_DEBUG 0

std::ostream& dev::eth::operator<<(std::ostream& _out, ExecutionResult const& _er)
{
	_out << "{" << _er.gasUsed << ", " << _er.newAddress << ", " << toHex(_er.output) << "}";
	return _out;
}

TransactionException dev::eth::toTransactionException(Exception const& _e)
{
	// Basic Transaction exceptions
	if (!!dynamic_cast<RLPException const*>(&_e))
		return TransactionException::BadRLP;
	if (!!dynamic_cast<OutOfGasIntrinsic const*>(&_e))
		return TransactionException::OutOfGasIntrinsic;
	if (!!dynamic_cast<InvalidSignature const*>(&_e))
		return TransactionException::InvalidSignature;
	// Executive exceptions
	if (!!dynamic_cast<OutOfGasBase const*>(&_e))
		return TransactionException::OutOfGasBase;
	if (!!dynamic_cast<InvalidNonce const*>(&_e))
		return TransactionException::InvalidNonce;
	if (!!dynamic_cast<NotEnoughCash const*>(&_e))
		return TransactionException::NotEnoughCash;
	if (!!dynamic_cast<BlockGasLimitReached const*>(&_e))
		return TransactionException::BlockGasLimitReached;
	if (!!dynamic_cast<AddressAlreadyUsed const*>(&_e))
		return TransactionException::AddressAlreadyUsed;
	// VM execution exceptions
	if (!!dynamic_cast<BadInstruction const*>(&_e))
		return TransactionException::BadInstruction;
	if (!!dynamic_cast<BadJumpDestination const*>(&_e))
		return TransactionException::BadJumpDestination;
	if (!!dynamic_cast<OutOfGas const*>(&_e))
		return TransactionException::OutOfGas;
	if (!!dynamic_cast<OutOfStack const*>(&_e))
		return TransactionException::OutOfStack;
	if (!!dynamic_cast<StackUnderflow const*>(&_e))
		return TransactionException::StackUnderflow;
	return TransactionException::Unknown;
}

std::ostream& dev::eth::operator<<(std::ostream& _out, TransactionException const& _er)
{
	switch (_er)
	{
		case TransactionException::None: _out << "None"; break;
		case TransactionException::BadRLP: _out << "BadRLP"; break;
		case TransactionException::InvalidFormat: _out << "InvalidFormat"; break;
		case TransactionException::OutOfGasIntrinsic: _out << "OutOfGasIntrinsic"; break;
		case TransactionException::InvalidSignature: _out << "InvalidSignature"; break;
		case TransactionException::InvalidNonce: _out << "InvalidNonce"; break;
		case TransactionException::NotEnoughCash: _out << "NotEnoughCash"; break;
		case TransactionException::OutOfGasBase: _out << "OutOfGasBase"; break;
		case TransactionException::BlockGasLimitReached: _out << "BlockGasLimitReached"; break;
		case TransactionException::BadInstruction: _out << "BadInstruction"; break;
		case TransactionException::BadJumpDestination: _out << "BadJumpDestination"; break;
		case TransactionException::OutOfGas: _out << "OutOfGas"; break;
		case TransactionException::OutOfStack: _out << "OutOfStack"; break;
		case TransactionException::StackUnderflow: _out << "StackUnderflow"; break;
        case TransactionException::RevertInstruction:
            _out << "RevertInstruction";
            break;
        default: _out << "Unknown"; break;
	}
	return _out;
}

Transaction::Transaction(bytesConstRef _rlpData, CheckTransaction _checkSig):
	TransactionBase(_rlpData, _checkSig)
{
}
