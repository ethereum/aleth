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

#include "EthashAux.h"
#include "Ethash.h"

#include <libdevcore/Common.h>
#include <libdevcore/FileSystem.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/CryptoPP.h>
#include <libethash/internal.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Exceptions.h>

#include <ethash/ethash.hpp>

#include <boost/detail/endian.hpp>
#include <boost/filesystem.hpp>

#include <array>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;
using namespace dev;
using namespace eth;

EthashAux* dev::eth::EthashAux::s_this = nullptr;

EthashAux::~EthashAux()
{
}

EthashAux* EthashAux::get()
{
    static std::once_flag flag;
    std::call_once(flag, []{s_this = new EthashAux();});
    return s_this;
}

static uint64_t number(h256 const& _seedHash)
{
    int epoch = ethash::find_epoch_number(ethash::hash256_from_bytes(_seedHash.data()));
    return static_cast<uint64_t>(epoch) * ethash::epoch_length;
}

EthashAux::LightType EthashAux::light(h256 const& _seedHash)
{
    UpgradableGuard l(get()->x_lights);
    if (get()->m_lights.count(_seedHash))
        return get()->m_lights.at(_seedHash);
    UpgradeGuard l2(l);
    return (get()->m_lights[_seedHash] = make_shared<LightAllocation>(_seedHash));
}

EthashAux::LightAllocation::LightAllocation(h256 const& _seedHash)
{
    uint64_t blockNumber = number(_seedHash);
    light = ethash_light_new(blockNumber);
    if (!light)
        BOOST_THROW_EXCEPTION(ExternalFunctionFailure() << errinfo_externalFunction("ethash_light_new()"));
    size = ethash_get_cachesize(blockNumber);
}

EthashAux::LightAllocation::~LightAllocation()
{
    ethash_light_delete(light);
}

bytesConstRef EthashAux::LightAllocation::data() const
{
    return bytesConstRef((byte const*)light->cache, size);
}

EthashAux::FullAllocation::FullAllocation(ethash_light_t _light, ethash_callback_t _cb)
{
//	cdebug << "About to call ethash_full_new...";
    full = ethash_full_new(_light, _cb);
//	cdebug << "Called OK.";
    if (!full)
    {
        clog(VerbosityWarning, "DAG") << "DAG Generation Failure. Reason: " << strerror(errno);
        BOOST_THROW_EXCEPTION(ExternalFunctionFailure() << errinfo_externalFunction("ethash_full_new"));
    }
}

EthashAux::FullAllocation::~FullAllocation()
{
    ethash_full_delete(full);
}

bytesConstRef EthashAux::FullAllocation::data() const
{
    return bytesConstRef((byte const*)ethash_full_dag(full), size());
}

static std::function<int(unsigned)> s_dagCallback;
static int dagCallbackShim(unsigned _p)
{
    clog(VerbosityInfo, "DAG") << "Generating DAG file. Progress: " << toString(_p) << "%";
    return s_dagCallback ? s_dagCallback(_p) : 0;
}

EthashAux::FullType EthashAux::full(h256 const& _seedHash, bool _createIfMissing, function<int(unsigned)> const& _f)
{
    FullType ret;
    auto l = light(_seedHash);

    DEV_GUARDED(get()->x_fulls)
        if ((ret = get()->m_fulls[_seedHash].lock()))
        {
            get()->m_lastUsedFull = ret;
            return ret;
        }

    if (_createIfMissing || computeFull(_seedHash, false) == 100)
    {
        s_dagCallback = _f;
//		cnote << "Loading from libethash...";
        ret = make_shared<FullAllocation>(l->light, dagCallbackShim);
//		cnote << "Done loading.";

        DEV_GUARDED(get()->x_fulls)
            get()->m_fulls[_seedHash] = get()->m_lastUsedFull = ret;
    }

    return ret;
}

unsigned EthashAux::computeFull(h256 const& _seedHash, bool _createIfMissing)
{
    Guard l(get()->x_fulls);
    uint64_t blockNumber = number(_seedHash);

    if (FullType ret = get()->m_fulls[_seedHash].lock())
    {
        get()->m_lastUsedFull = ret;
        return 100;
    }

    if (_createIfMissing && (!get()->m_fullGenerator || !get()->m_fullGenerator->joinable()))
    {
        get()->m_fullProgress = 0;
        get()->m_generatingFullNumber = blockNumber / ETHASH_EPOCH_LENGTH * ETHASH_EPOCH_LENGTH;
        get()->m_fullGenerator = unique_ptr<thread>(new thread([=](){
            cnote << "Loading full DAG of seedhash: " << _seedHash;
            get()->full(_seedHash, true, [](unsigned p){ get()->m_fullProgress = p; return 0; });
            cnote << "Full DAG loaded";
            get()->m_fullProgress = 0;
            get()->m_generatingFullNumber = NotGenerating;
        }));
    }

    return (get()->m_generatingFullNumber == blockNumber) ? get()->m_fullProgress : 0;
}
