// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/math/distributions/normal.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "BasicGasPricer.h"
#include "BlockChain.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

void BasicGasPricer::update(BlockChain const& _bc)
{
	unsigned c = 0;
	h256 p = _bc.currentHash();
	m_gasPerBlock = _bc.info(p).gasLimit();

	map<u256, u256> dist;
	u256 total = 0;

	// make gasPrice versus gasUsed distribution for the last 1000 blocks
	while (c < 1000 && p)
	{
		BlockHeader bi = _bc.info(p);
		if (bi.transactionsRoot() != EmptyTrie)
		{
			auto bb = _bc.block(p);
			RLP r(bb);
			BlockReceipts brs(_bc.receipts(bi.hash()));
			size_t i = 0;
			for (auto const& tr: r[1])
			{
				Transaction tx(tr.data(), CheckTransaction::None);
				u256 gu = brs.receipts[i].cumulativeGasUsed();
				dist[tx.gasPrice()] += gu;
				total += gu;
				i++;
			}
		}
		p = bi.parentHash();
		++c;
	}

	// fill m_octiles with weighted gasPrices
	if (total > 0)
	{
		m_octiles[0] = dist.begin()->first;

		// calc mean
		u256 mean = 0;
		for (auto const& i: dist)
			mean += i.first * i.second;
		mean /= total;

		// calc standard deviation
		u256 sdSquared = 0;
		for (auto const& i: dist)
			sdSquared += i.second * (i.first - mean) * (i.first - mean);
		sdSquared /= total;

		if (sdSquared)
		{
			long double sd = sqrt(sdSquared.convert_to<long double>());
			long double normalizedSd = sd / mean.convert_to<long double>();

			// calc octiles normalized to gaussian distribution
			boost::math::normal gauss(1.0, (normalizedSd > 0.01) ? normalizedSd : 0.01);
			for (size_t i = 1; i < 8; i++)
				m_octiles[i] = u256(mean.convert_to<long double>() * boost::math::quantile(gauss, i / 8.0));
			m_octiles[8] = dist.rbegin()->first;
		}
		else
		{
			for (size_t i = 0; i < 9; i++)
				m_octiles[i] = (i + 1) * mean / 5;
		}
	}
}
