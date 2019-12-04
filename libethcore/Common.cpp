// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "Common.h"
#include <libdevcore/Base64.h>
#include <libdevcore/Terminal.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include "Exceptions.h"
#include "BlockHeader.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace eth
{

const unsigned c_protocolVersion = 63;
#if ETH_FATDB
const unsigned c_databaseMinorVersion = 4;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 1;
#else
const unsigned c_databaseMinorVersion = 3;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 0;
#endif

const unsigned c_databaseVersion = c_databaseBaseVersion + (c_databaseVersionModifier << 8) + (23 << 9);

const Address c_blockhashContractAddress(0xf0);
const bytes c_blockhashContractCode(fromHex("0x600073fffffffffffffffffffffffffffffffffffffffe33141561005957600143035b60011561005357600035610100820683015561010081061561004057005b6101008104905061010082019150610022565b506100e0565b4360003512156100d4576000356001814303035b61010081121515610085576000610100830614610088565b60005b156100a75761010083019250610100820491506101008104905061006d565b610100811215156100bd57600060a052602060a0f35b610100820683015460c052602060c0f350506100df565b600060e052602060e0f35b5b50"));

Address toAddress(std::string const& _s)
{
	try
	{
		auto b = fromHex(_s.substr(0, 2) == "0x" ? _s.substr(2) : _s, WhenError::Throw);
		if (b.size() == 20)
			return Address(b);
	}
	catch (BadHexCharacter&) {}
	BOOST_THROW_EXCEPTION(InvalidAddress());
}

vector<pair<u256, string>> const& units()
{
	static const vector<pair<u256, string>> s_units =
	{
		{exp10<54>(), "Uether"},
		{exp10<51>(), "Vether"},
		{exp10<48>(), "Dether"},
		{exp10<45>(), "Nether"},
		{exp10<42>(), "Yether"},
		{exp10<39>(), "Zether"},
		{exp10<36>(), "Eether"},
		{exp10<33>(), "Pether"},
		{exp10<30>(), "Tether"},
		{exp10<27>(), "Gether"},
		{exp10<24>(), "Mether"},
		{exp10<21>(), "grand"},
		{exp10<18>(), "ether"},
		{exp10<15>(), "finney"},
		{exp10<12>(), "szabo"},
		{exp10<9>(), "Gwei"},
		{exp10<6>(), "Mwei"},
		{exp10<3>(), "Kwei"},
		{exp10<0>(), "wei"}
	};

	return s_units;
}

std::string formatBalance(bigint const& _b)
{
	ostringstream ret;
	u256 b;
	if (_b < 0)
	{
		ret << "-";
		b = (u256)-_b;
	}
	else
		b = (u256)_b;

	if (b > units()[0].first * 1000)
	{
		ret << (b / units()[0].first) << " " << units()[0].second;
		return ret.str();
	}
	ret << setprecision(5);
	for (auto const& i: units())
		if (i.first != 1 && b >= i.first)
		{
			ret << (double(b / (i.first / 1000)) / 1000.0) << " " << i.second;
			return ret.str();
		}
	ret << b << " wei";
	return ret.str();
}

static void badBlockInfo(BlockHeader const& _bi, string const& _err)
{
	string const c_line = EthReset EthOnMaroon + string(80, ' ') + EthReset;
	string const c_border = EthReset EthOnMaroon + string(2, ' ') + EthReset EthMaroonBold;
	string const c_space = c_border + string(76, ' ') + c_border + EthReset;
	stringstream ss;
	ss << c_line << "\n";
	ss << c_space << "\n";
	ss << c_border + "  Import Failure     " + _err + string(max<int>(0, 53 - _err.size()), ' ') + "  " + c_border << "\n";
	ss << c_space << "\n";
	string bin = toString(_bi.number());
	ss << c_border + ("                     Bad Block #" + string(max<int>(0, 8 - bin.size()), '0') + bin + "." + _bi.hash().abridged() + "                    ") + c_border << "\n";
	ss << c_space << "\n";
	ss << c_line;
	cwarn << "\n" + ss.str();
}

void badBlock(bytesConstRef _block, string const& _err)
{
	BlockHeader bi;
	DEV_IGNORE_EXCEPTIONS(bi = BlockHeader(_block));
	badBlockInfo(bi, _err);
}

string TransactionSkeleton::userReadable(bool _toProxy, function<pair<bool, string>(TransactionSkeleton const&)> const& _getNatSpec, function<string(Address const&)> const& _formatAddress) const
{
	if (creation)
	{
		// show notice concerning the creation code. TODO: this needs entering into natspec.
		return string("ÐApp is attempting to create a contract; ") + (_toProxy ? "(this transaction is not executed directly, but forwarded to another ÐApp) " : "") + "to be endowed with " + formatBalance(value) + ", with additional network fees of up to " + formatBalance(gas * gasPrice) + ".\n\nMaximum total cost is " + formatBalance(value + gas * gasPrice) + ".";
	}

	bool isContract;
	std::string natSpec;
	tie(isContract, natSpec) = _getNatSpec(*this);
	if (!isContract)
	{
		// recipient has no code - nothing special about this transaction, show basic value transfer info
		return "ÐApp is attempting to send " + formatBalance(value) + " to a recipient " + _formatAddress(to) + (_toProxy ? " (this transaction is not executed directly, but forwarded to another ÐApp)" : "") + ", with additional network fees of up to " + formatBalance(gas * gasPrice) + ".\n\nMaximum total cost is " + formatBalance(value + gas * gasPrice) + ".";
	}

	if (natSpec.empty())
		return "ÐApp is attempting to call into an unknown contract at address " +
				_formatAddress(to) + ".\n\n" +
				(_toProxy ? "This transaction is not executed directly, but forwarded to another ÐApp.\n\n" : "")  +
				"Call involves sending " +
				formatBalance(value) + " to the recipient, with additional network fees of up to " +
				formatBalance(gas * gasPrice) +
				"However, this also does other stuff which we don't understand, and does so in your name.\n\n" +
				"WARNING: This is probably going to cost you at least " +
				formatBalance(value + gas * gasPrice) +
				", however this doesn't include any side-effects, which could be of far greater importance.\n\n" +
				"REJECT UNLESS YOU REALLY KNOW WHAT YOU ARE DOING!";

	return "ÐApp attempting to conduct contract interaction with " +
	_formatAddress(to) +
	": <b>" + natSpec + "</b>.\n\n" +
	(_toProxy ? "This transaction is not executed directly, but forwarded to another ÐApp.\n\n" : "") +
	(value > 0 ?
		"In addition, ÐApp is attempting to send " +
		formatBalance(value) + " to said recipient, with additional network fees of up to " +
		formatBalance(gas * gasPrice) + " = " +
		formatBalance(value + gas * gasPrice) + "."
	:
		"Additional network fees are at most" +
		formatBalance(gas * gasPrice) + ".");
}

}
}
