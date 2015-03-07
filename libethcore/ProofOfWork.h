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
/** @file ProofOfWork.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * ProofOfWork algorithm. Or not.
 */

#pragma once

#include <chrono>
#include <thread>
#include <cstdint>
#include <libdevcrypto/SHA3.h>
#include "Common.h"
#include "BlockInfo.h"

#define FAKE_DAGGER 1

namespace dev
{
namespace eth
{

struct MineInfo
{
	void combine(MineInfo const& _m) { requirement = std::max(requirement, _m.requirement); best = std::min(best, _m.best); hashes += _m.hashes; completed = completed || _m.completed; }
	double requirement = 0;
	double best = 1e99;
	unsigned hashes = 0;
	bool completed = false;
};

class Ethash
{
public:
	struct Proof
	{
		Nonce nonce;
		h256 mixHash;
	};

	static bool verify(BlockInfo const& _header);
	std::pair<MineInfo, Proof> mine(BlockInfo const& _header, unsigned _msTimeout = 100, bool _continue = true, bool _turbo = false);
	static void assignResult(Proof const& _r, BlockInfo& _header) { _header.nonce = _r.nonce; _header.mixHash = _r.mixHash; }

protected:
	Nonce m_last;
};

template <class Evaluator>
class ProofOfWorkEngine: public Evaluator
{
public:
	using Proof = Nonce;

	static bool verify(BlockInfo const& _header) { return (bigint)(u256)Evaluator::eval(_header.headerHash(WithoutNonce), _header.nonce) <= (bigint(1) << 256) / _header.difficulty; }
	inline std::pair<MineInfo, Proof> mine(BlockInfo const& _header, unsigned _msTimeout = 100, bool _continue = true, bool _turbo = false);
	static void assignResult(Proof const& _r, BlockInfo& _header) { _header.nonce = _r; }

protected:
	Nonce m_last;
};

class SHA3Evaluator
{
public:
	static h256 eval(h256 const& _root, Nonce const& _nonce) { h256 b[2] = { _root, h256(_nonce) }; return sha3(bytesConstRef((byte const*)&b[0], 64)); }
};

using SHA3ProofOfWork = ProofOfWorkEngine<SHA3Evaluator>;

using ProofOfWork = Ethash;

template <class Evaluator>
std::pair<MineInfo, typename ProofOfWorkEngine<Evaluator>::Proof> ProofOfWorkEngine<Evaluator>::mine(BlockInfo const& _header, unsigned _msTimeout, bool _continue, bool _turbo)
{
	auto headerHashWithoutNonce = _header.headerHash(WithoutNonce);
	auto difficulty = _header.difficulty;

	std::pair<MineInfo, Nonce> ret;
	static std::mt19937_64 s_eng((time(0) + *reinterpret_cast<unsigned*>(m_last.data())));
	Nonce::Arith s = (m_last = Nonce::random(s_eng));

	bigint d = (bigint(1) << 256) / difficulty;
	ret.first.requirement = log2((double)d);

	// 2^ 0      32      64      128      256
	//   [--------*-------------------------]
	//
	// evaluate until we run out of time
	auto startTime = std::chrono::steady_clock::now();
	if (!_turbo)
		std::this_thread::sleep_for(std::chrono::milliseconds(_msTimeout * 90 / 100));
	double best = 1e99;	// high enough to be effectively infinity :)
	ProofOfWorkEngine<Evaluator>::Proof solution;
	unsigned h = 0;
	for (; (std::chrono::steady_clock::now() - startTime) < std::chrono::milliseconds(_msTimeout) && _continue; s++, h++)
	{
		solution = (ProofOfWorkEngine<Evaluator>::Proof)s;
		auto e = (bigint)(u256)Evaluator::eval(headerHashWithoutNonce, solution);
		best = std::min<double>(best, log2((double)e));
		if (e <= d)
		{
			ret.first.completed = true;
			break;
		}
	}
	ret.first.hashes = h;
	ret.first.best = best;
	ret.second = solution;

	if (ret.first.completed)
	{
		BlockInfo test = _header;
		assignResult(solution, test);
		assert(verify(test));
	}

	return ret;
}

}
}
