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
/** @file Transaction.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>
#include <libethcore/Transaction.h>
#include <libevmcore/Params.h>

namespace dev
{
namespace eth
{

enum class TransactionException
{
	None = 0,
	Unknown,
	BadRLP,
	InvalidFormat,
	OutOfGasIntrinsic,		///< Too little gas to pay for the base transaction cost.
	InvalidSignature,
	InvalidNonce,
	NotEnoughCash,
	OutOfGasBase,			///< Too little gas to pay for the base transaction cost.
	BlockGasLimitReached,
	BadInstruction,
	BadJumpDestination,
	OutOfGas,				///< Ran out of gas executing code of the transaction.
	OutOfStack,				///< Ran out of stack executing code of the transaction.
	StackUnderflow
};

enum class CodeDeposit
{
	None = 0,
	Failed,
	Success
};

struct VMException;

TransactionException toTransactionException(Exception const& _e);
std::ostream& operator<<(std::ostream& _out, TransactionException const& _er);

/// Description of the result of executing a transaction.
struct ExecutionResult
{
	u256 gasUsed = 0;
	TransactionException excepted = TransactionException::Unknown;
	Address newAddress;
	bytes output;
	CodeDeposit codeDeposit = CodeDeposit::None;					///< Failed if an attempted deposit failed due to lack of gas.
	u256 gasRefunded = 0;
	unsigned depositSize = 0; 										///< Amount of code of the creation's attempted deposit.
	u256 gasForDeposit; 											///< Amount of gas remaining for the code deposit phase.
};

std::ostream& operator<<(std::ostream& _out, ExecutionResult const& _er);

/// Encodes a transaction, ready to be exported to or freshly imported from RLP.
class Transaction: public TransactionBase
{
public:
	/// Constructs a null transaction.
	Transaction() {}

	using TransactionBase::TransactionBase;

	/// Constructs a transaction from the given RLP.
	explicit Transaction(bytesConstRef _rlp, CheckTransaction _checkSig);

	/// Constructs a transaction from the given RLP.
	explicit Transaction(bytes const& _rlp, CheckTransaction _checkSig): Transaction(&_rlp, _checkSig) {}

	/// @returns true if the transaction contains enough gas for the basic payment.
	bool checkPayment() const { return m_gas >= gasRequired(); }

	/// @returns the gas required to run this transaction.
	bigint gasRequired() const;

	/// Get the fee associated for a transaction with the given data.
	template <class T> static bigint gasRequired(T const& _data, u256 _gas = 0) { bigint ret = c_txGas + _gas; for (auto i: _data) ret += i ? c_txDataNonZeroGas : c_txDataZeroGas; return ret; }

private:
	mutable bigint m_gasRequired = 0;	///< Memoised amount required for the transaction to run.
};

/// Nice name for vector of Transaction.
using Transactions = std::vector<Transaction>;

}
}
