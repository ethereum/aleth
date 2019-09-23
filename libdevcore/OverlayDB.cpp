// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include <thread>
#include <libdevcore/db.h>
#include <libdevcore/Common.h>
#include "SHA3.h"
#include "OverlayDB.h"
#include "TrieDB.h"

namespace dev
{
namespace
{
inline db::Slice toSlice(h256 const& _h)
{
    return db::Slice(reinterpret_cast<char const*>(_h.data()), _h.size);
}

inline db::Slice toSlice(std::string const& _str)
{
    return db::Slice(_str.data(), _str.size());
}

inline db::Slice toSlice(bytes const& _b)
{
    return db::Slice(reinterpret_cast<char const*>(&_b[0]), _b.size());
}

}  // namespace

OverlayDB::~OverlayDB() = default;

void OverlayDB::commit()
{
    if (m_db)
    {
        auto writeBatch = m_db->createWriteBatch();
//      cnote << "Committing nodes to disk DB:";
#if DEV_GUARDED_DB
        DEV_READ_GUARDED(x_this)
#endif
        {
            for (auto const& i: m_main)
            {
                if (i.second.second)
                    writeBatch->insert(toSlice(i.first), toSlice(i.second.first));
//              cnote << i.first << "#" << m_main[i.first].second;
            }
            for (auto const& i: m_aux)
                if (i.second.second)
                {
                    bytes b = i.first.asBytes();
                    b.push_back(255);   // for aux
                    writeBatch->insert(toSlice(b), toSlice(i.second.first));
                }
        }

        for (unsigned i = 0; i < 10; ++i)
        {
            try
            {
                m_db->commit(std::move(writeBatch));
                break;
            }
            catch (boost::exception const& ex)
            {
                if (i == 9)
                {
                    cwarn << "Fail writing to state database. Bombing out.";
                    exit(-1);
                }
                cwarn << "Error writing to state database: " << boost::diagnostic_information(ex);
                cwarn << "Sleeping for" << (i + 1) << "seconds, then retrying.";
                std::this_thread::sleep_for(std::chrono::seconds(i + 1));
            }
        }
#if DEV_GUARDED_DB
        DEV_WRITE_GUARDED(x_this)
#endif
        {
            m_aux.clear();
            m_main.clear();
        }
    }
}

bytes OverlayDB::lookupAux(h256 const& _h) const
{
    bytes ret = StateCacheDB::lookupAux(_h);
    if (!ret.empty() || !m_db)
        return ret;

    bytes b = _h.asBytes();
    b.push_back(255);   // for aux
    std::string const v = m_db->lookup(toSlice(b));
    if (v.empty())
        cwarn << "Aux not found: " << _h;

    return asBytes(v);
}

void OverlayDB::rollback()
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_main.clear();
}

std::string OverlayDB::lookup(h256 const& _h) const
{
    std::string ret = StateCacheDB::lookup(_h);
    if (!ret.empty() || !m_db)
        return ret;

    return m_db->lookup(toSlice(_h));
}

bool OverlayDB::exists(h256 const& _h) const
{
    if (StateCacheDB::exists(_h))
        return true;
    return m_db && m_db->exists(toSlice(_h));
}

void OverlayDB::kill(h256 const& _h)
{
    if (!StateCacheDB::kill(_h))
    {
        if (m_db)
        {
            if (!m_db->exists(toSlice(_h)))
            {
                // No point node ref decreasing for EmptyTrie since we never bother incrementing it
                // in the first place for empty storage tries.
                if (_h != EmptyTrie)
                    cnote << "Decreasing DB node ref count below zero with no DB node. Probably "
                             "have a corrupt Trie."
                          << _h;
                // TODO: for 1.1: ref-counted triedb.
            }
        }
    }
}

}
