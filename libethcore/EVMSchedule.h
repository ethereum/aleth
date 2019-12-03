// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libdevcore/Common.h>
#include <libethcore/Common.h>
#include <boost/optional.hpp>
#include <array>

namespace dev
{
namespace eth
{
struct AdditionalEIPs;

struct EVMSchedule
{
    EVMSchedule(): tierStepGas(std::array<unsigned, 8>{{0, 2, 3, 5, 8, 10, 20, 0}}) {}
    EVMSchedule(bool _efcd, bool _hdc, unsigned const& _txCreateGas): exceptionalFailedCodeDeposit(_efcd), haveDelegateCall(_hdc), tierStepGas(std::array<unsigned, 8>{{0, 2, 3, 5, 8, 10, 20, 0}}), txCreateGas(_txCreateGas) {}
    // construct schedule with additional EIPs on top
    EVMSchedule(EVMSchedule const& _schedule, AdditionalEIPs const& _eips);
    unsigned accountVersion = 0;
    bool exceptionalFailedCodeDeposit = true;
    bool haveDelegateCall = true;
    bool eip150Mode = false;
    bool eip158Mode = false;
    bool eip1283Mode = false;
    bool eip2200Mode = false;
    bool haveBitwiseShifting = false;
    bool haveRevert = false;
    bool haveReturnData = false;
    bool haveStaticCall = false;
    bool haveCreate2 = false;
    bool haveExtcodehash = false;
    bool haveChainID = false;
    bool haveSelfbalance = false;
    std::array<unsigned, 8> tierStepGas;
    unsigned expGas = 10;
    unsigned expByteGas = 10;
    unsigned sha3Gas = 30;
    unsigned sha3WordGas = 6;
    unsigned sloadGas = 50;
    unsigned sstoreSetGas = 20000;
    unsigned sstoreResetGas = 5000;
    unsigned sstoreUnchangedGas = 200;
    unsigned sstoreRefundGas = 15000;
    unsigned jumpdestGas = 1;
    unsigned logGas = 375;
    unsigned logDataGas = 8;
    unsigned logTopicGas = 375;
    unsigned createGas = 32000;
    unsigned callGas = 40;
    unsigned precompileStaticCallGas = 700;
    unsigned callSelfGas = 40;
    unsigned callStipend = 2300;
    unsigned callValueTransferGas = 9000;
    unsigned callNewAccountGas = 25000;
    unsigned selfdestructRefundGas = 24000;
    unsigned memoryGas = 3;
    unsigned quadCoeffDiv = 512;
    unsigned createDataGas = 200;
    unsigned txGas = 21000;
    unsigned txCreateGas = 53000;
    unsigned txDataZeroGas = 4;
    unsigned txDataNonZeroGas = 68;
    unsigned copyGas = 3;

    unsigned extcodesizeGas = 20;
    unsigned extcodecopyGas = 20;
    unsigned extcodehashGas = 400;
    unsigned balanceGas = 20;
    unsigned selfdestructGas = 0;
    unsigned blockhashGas = 20;
    unsigned maxCodeSize = unsigned(-1);

    boost::optional<u256> blockRewardOverwrite;

    bool staticCallDepthLimit() const { return !eip150Mode; }
    bool emptinessIsNonexistence() const { return eip158Mode; }
    bool zeroValueTransferChargesNewAccountGas() const { return !eip158Mode; }
    bool sstoreNetGasMetering() const { return eip1283Mode || eip2200Mode; }
    bool sstoreThrowsIfGasBelowCallStipend() const { return eip2200Mode; }
};

static const EVMSchedule DefaultSchedule = EVMSchedule();
static const EVMSchedule FrontierSchedule = EVMSchedule(false, false, 21000);
static const EVMSchedule HomesteadSchedule = EVMSchedule(true, true, 53000);

static const EVMSchedule EIP150Schedule = []
{
    EVMSchedule schedule = HomesteadSchedule;
    schedule.eip150Mode = true;
    schedule.extcodesizeGas = 700;
    schedule.extcodecopyGas = 700;
    schedule.balanceGas = 400;
    schedule.sloadGas = 200;
    schedule.callGas = 700;
    schedule.callSelfGas = 700;
    schedule.selfdestructGas = 5000;
    return schedule;
}();

static const EVMSchedule EIP158Schedule = []
{
    EVMSchedule schedule = EIP150Schedule;
    schedule.expByteGas = 50;
    schedule.eip158Mode = true;
    schedule.maxCodeSize = 0x6000;
    return schedule;
}();

static const EVMSchedule ByzantiumSchedule = []
{
    EVMSchedule schedule = EIP158Schedule;
    schedule.haveRevert = true;
    schedule.haveReturnData = true;
    schedule.haveStaticCall = true;
    schedule.blockRewardOverwrite = {3 * ether};
    return schedule;
}();

static const EVMSchedule EWASMSchedule = []
{
    EVMSchedule schedule = ByzantiumSchedule;
    schedule.maxCodeSize = std::numeric_limits<unsigned>::max();
    return schedule;
}();

static const EVMSchedule ConstantinopleSchedule = []
{
    EVMSchedule schedule = ByzantiumSchedule;
    schedule.haveCreate2 = true;
    schedule.haveBitwiseShifting = true;
    schedule.haveExtcodehash = true;
    schedule.eip1283Mode = true;
    schedule.blockRewardOverwrite = {2 * ether};
    return schedule;
}();

static const EVMSchedule ConstantinopleFixSchedule = [] {
    EVMSchedule schedule = ConstantinopleSchedule;
    schedule.eip1283Mode = false;
    return schedule;
}();

static const EVMSchedule IstanbulSchedule = [] {
    EVMSchedule schedule = ConstantinopleFixSchedule;
    schedule.txDataNonZeroGas = 16;
    schedule.sloadGas = 800;
    schedule.balanceGas = 700;
    schedule.extcodehashGas = 700;
    schedule.haveChainID = true;
    schedule.haveSelfbalance = true;
    schedule.eip2200Mode = true;
    schedule.sstoreUnchangedGas = 800;
    return schedule;
}();

static const EVMSchedule& MuirGlacierSchedule = IstanbulSchedule;

static const EVMSchedule BerlinSchedule = [] {
    EVMSchedule schedule = MuirGlacierSchedule;
    return schedule;
}();

static const EVMSchedule ExperimentalSchedule = [] {
    EVMSchedule schedule = BerlinSchedule;
    schedule.accountVersion = 1;
    schedule.blockhashGas = 800;
    return schedule;
}();

inline EVMSchedule const& latestScheduleForAccountVersion(u256 const& _version)
{
    if (_version == 0)
        return IstanbulSchedule;
    else if (_version == ExperimentalSchedule.accountVersion)
        return ExperimentalSchedule;
    else
    {
        // This should not happen, as all existing accounts
        // are created either with version 0 or with one of fork's versions
        assert(false);
        return DefaultSchedule;
    }
}
}
}
