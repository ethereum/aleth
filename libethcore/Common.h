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
/** @file Common.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Ethereum-specific data structures & algorithms.
 */

#pragma once

#include <string>
#include <functional>
#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcrypto/Common.h>

namespace dev
{
namespace eth
{

/// Current protocol version.
extern const unsigned c_protocolVersion;

/// Current minor protocol version.
extern const unsigned c_minorProtocolVersion;

/// Current database version.
extern const unsigned c_databaseVersion;

/// The network id.
enum class Network
{
	Olympic = 0,
	Frontier = 1,
	Turbo = 2
};
extern Network c_network;

Network resetNetwork(Network _n);

/// User-friendly string representation of the amount _b in wei.
std::string formatBalance(bigint const& _b);

/// Get information concerning the currency denominations.
std::vector<std::pair<u256, std::string>> const& units();

/// The log bloom's size (2048-bit).
using LogBloom = h2048;

/// Many log blooms.
using LogBlooms = std::vector<LogBloom>;

template <size_t n> inline u256 exp10()
{
	return exp10<n - 1>() * u256(10);
}

template <> inline u256 exp10<0>()
{
	return u256(1);
}

// The various denominations; here for ease of use where needed within code.
static const u256 ether = exp10<18>();
static const u256 finney = exp10<15>();
static const u256 szabo = exp10<12>();
static const u256 wei = exp10<0>();

using Nonce = h64;

using BlockNumber = unsigned;

static const BlockNumber LatestBlock = (BlockNumber)-2;
static const BlockNumber PendingBlock = (BlockNumber)-1;
static const h256 LatestBlockHash = h256(2);
static const h256 EarliestBlockHash = h256(1);
static const h256 PendingBlockHash = h256(0);


enum class RelativeBlock: BlockNumber
{
	Latest = LatestBlock,
	Pending = PendingBlock
};

class Transaction;

struct ImportRoute
{
	h256s deadBlocks;
	h256s liveBlocks;
	std::vector<Transaction> goodTranactions;
};

enum class ImportResult
{
	Success = 0,
	UnknownParent,
	FutureTimeKnown,
	FutureTimeUnknown,
	AlreadyInChain,
	AlreadyKnown,
	Malformed,
	OverbidGasPrice,
	BadChain
};

struct ImportRequirements
{
	using value = unsigned;
	enum
	{
		ValidSeal = 1, ///< Validate seal
		DontHave = 2, ///< Avoid old blocks
		UncleBasic = 4, ///< Check the basic structure of the uncles.
		TransactionBasic = 8, ///< Check the basic structure of the transactions.
		UncleSeals = 16, ///< Check the basic structure of the uncles.
		TransactionSignatures = 32, ///< Check the basic structure of the transactions.
		Parent = 64, ///< Check parent block header
		CheckUncles = UncleBasic | UncleSeals, ///< Check uncle seals
		CheckTransactions = TransactionBasic | TransactionSignatures, ///< Check transaction signatures
		Everything = ValidSeal | DontHave | CheckUncles | CheckTransactions | Parent,
		None = 0
	};
};

/// Super-duper signal mechanism. TODO: replace with somthing a bit heavier weight.
template<typename... Args> class Signal
{
public:
	using Callback = std::function<void(Args...)>;

	class HandlerAux
	{
		friend class Signal;

	public:
		~HandlerAux() { if (m_s) m_s->m_fire.erase(m_i); m_s = nullptr; }
		void reset() { m_s = nullptr; }
		void fire(Args&&... _args) { m_h(std::forward<Args>(_args)...); }

	private:
		HandlerAux(unsigned _i, Signal* _s, Callback const& _h): m_i(_i), m_s(_s), m_h(_h) {}

		unsigned m_i = 0;
		Signal* m_s = nullptr;
		Callback m_h;
	};

	~Signal()
	{
		for (auto const& h : m_fire)
			if (auto l = h.second.lock())
				l->reset();
	}

	std::shared_ptr<HandlerAux> add(Callback const& _h)
	{
		auto n = m_fire.empty() ? 0 : (m_fire.rbegin()->first + 1);
		auto h =  std::shared_ptr<HandlerAux>(new HandlerAux(n, this, _h));
		m_fire[n] = h;
		return h;
	}

	void operator()(Args&... _args)
	{
		for (auto const& f: m_fire)
			if (auto h = f.second.lock())
				h->fire(std::forward<Args>(_args)...);
	}

private:
	std::map<unsigned, std::weak_ptr<typename Signal::HandlerAux>> m_fire;
};

template<class... Args> using Handler = std::shared_ptr<typename Signal<Args...>::HandlerAux>;

struct TransactionSkeleton
{
	bool creation = false;
	Address from;
	Address to;
	u256 value;
	bytes data;
	u256 nonce = UndefinedU256;
	u256 gas = UndefinedU256;
	u256 gasPrice = UndefinedU256;
};

void badBlock(bytesConstRef _header, std::string const& _err);
inline void badBlock(bytes const& _header, std::string const& _err) { badBlock(&_header, _err); }

// TODO: move back into a mining subsystem and have it be accessible from Sealant only via a dynamic_cast.
/**
 * @brief Describes the progress of a mining operation.
 */
struct WorkingProgress
{
//	MiningProgress& operator+=(MiningProgress const& _mp) { hashes += _mp.hashes; ms = std::max(ms, _mp.ms); return *this; }
	uint64_t hashes = 0;		///< Total number of hashes computed.
	uint64_t ms = 0;			///< Total number of milliseconds of mining thus far.
	uint64_t rate() const { return ms == 0 ? 0 : hashes * 1000 / ms; }
};

}
}
