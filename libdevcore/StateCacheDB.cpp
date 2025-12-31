// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "StateCacheDB.h"
#include "Common.h"
#include "CommonData.h"
using namespace std;
using namespace dev;

namespace dev
{

std::unordered_map<h256, std::string> StateCacheDB::get() const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    std::unordered_map<h256, std::string> ret;
    for (auto const& i: m_main)
        if (!m_enforceRefs || i.second.second > 0)
            ret.insert(make_pair(i.first, i.second.first));
    return ret;
}

StateCacheDB& StateCacheDB::operator=(StateCacheDB const& _c)
{
    if (this == &_c)
        return *this;
#if DEV_GUARDED_DB
    ReadGuard l(_c.x_this);
    WriteGuard l2(x_this);
#endif
    m_main = _c.m_main;
    m_aux = _c.m_aux;
    return *this;
}

std::string StateCacheDB::lookup(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end())
    {
        if (!m_enforceRefs || it->second.second > 0)
            return it->second.first;
        else
            cwarn << "Lookup required for value with refcount == 0. This is probably a critical trie issue" << _h;
    }
    return std::string();
}

bool StateCacheDB::exists(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end() && (!m_enforceRefs || it->second.second > 0))
        return true;
    return false;
}

void StateCacheDB::insert(h256 const& _h, bytesConstRef _v)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end())
    {
        it->second.first = _v.toString();
        it->second.second++;
    }
    else
        m_main[_h] = make_pair(_v.toString(), 1);
}

bool StateCacheDB::kill(h256 const& _h)
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    if (m_main.count(_h))
    {
        if (m_main[_h].second > 0)
        {
            m_main[_h].second--;
            return true;
        }
    }
    return false;
}

bytes StateCacheDB::lookupAux(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_aux.find(_h);
    if (it != m_aux.end() && (!m_enforceRefs || it->second.second))
        return it->second.first;
    return bytes();
}

void StateCacheDB::removeAux(h256 const& _h)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_aux[_h].second = false;
}

void StateCacheDB::insertAux(h256 const& _h, bytesConstRef _v)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_aux[_h] = make_pair(_v.toBytes(), true);
}

void StateCacheDB::purge()
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    // purge m_main
    for (auto it = m_main.begin(); it != m_main.end(); )
        if (it->second.second)
            ++it;
        else
            it = m_main.erase(it);

    // purge m_aux
    for (auto it = m_aux.begin(); it != m_aux.end(); )
        if (it->second.second)
            ++it;
        else
            it = m_aux.erase(it);
}

h256Hash StateCacheDB::keys() const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    h256Hash ret;
    for (auto const& i: m_main)
        if (i.second.second)
            ret.insert(i.first);
    return ret;
}

}
