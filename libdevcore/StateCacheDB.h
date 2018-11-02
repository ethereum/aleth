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

#pragma once

#include "Common.h"
#include "Log.h"
#include "RLP.h"

namespace dev
{
class StateCacheDB
{
    friend class EnforceRefs;

public:
    StateCacheDB() {}
    StateCacheDB(StateCacheDB const& _c) { operator=(_c); }

    StateCacheDB& operator=(StateCacheDB const& _c);

    virtual ~StateCacheDB() = default;

    void clear()
    {
        m_main.clear();
        m_aux.clear();
    }  // WARNING !!!! didn't originally clear m_refCount!!!
    std::unordered_map<h256, std::string> get() const;

    std::string lookup(h256 const& _h) const;
    bool exists(h256 const& _h) const;
    void insert(h256 const& _h, bytesConstRef _v);
    bool kill(h256 const& _h);
    void purge();

    bytes lookupAux(h256 const& _h) const;
    void removeAux(h256 const& _h);
    void insertAux(h256 const& _h, bytesConstRef _v);

    h256Hash keys() const;

protected:
#if DEV_GUARDED_DB
    mutable SharedMutex x_this;
#endif
    std::unordered_map<h256, std::pair<std::string, unsigned>> m_main;
    std::unordered_map<h256, std::pair<bytes, bool>> m_aux;

    mutable bool m_enforceRefs = false;
};

class EnforceRefs
{
public:
    EnforceRefs(StateCacheDB const& _o, bool _r) : m_o(_o), m_r(_o.m_enforceRefs)
    {
        _o.m_enforceRefs = _r;
    }
    ~EnforceRefs() { m_o.m_enforceRefs = m_r; }

private:
    StateCacheDB const& m_o;
    bool m_r;
};

inline std::ostream& operator<<(std::ostream& _out, StateCacheDB const& _m)
{
    for (auto const& i : _m.get())
    {
        _out << i.first << ": ";
        _out << RLP(i.second);
        _out << " " << toHex(i.second);
        _out << std::endl;
    }
    return _out;
}

}  // namespace dev
