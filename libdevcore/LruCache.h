// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <list>
#include <unordered_map>

namespace dev
{
template <class Key, class Value>
class LruCache
{
    typedef Key key_type;
    typedef Value value_type;
    typedef std::list<std::pair<key_type, value_type>> list_type;
    typedef std::unordered_map<key_type, typename list_type::const_iterator> map_type;

public:
    explicit LruCache(size_t _capacity) : m_capacity(_capacity) {}

    LruCache(LruCache<key_type, value_type> const& _l)
    {
        m_capacity = _l.m_capacity;
        m_data = _l.m_data;
        m_index = _l.m_index;
    }

    LruCache(LruCache<key_type, value_type>&& _l)
    {
        m_capacity = _l.m_capacity;
        m_data = std::move(_l.m_data);
        m_index = std::move(_l.m_index);
    }

    LruCache<key_type, value_type>& operator=(LruCache const& _l)
    {
        m_capacity = _l.m_capacity;
        m_data = _l.m_data;
        m_index = _l.m_index;

        return *this;
    }

    LruCache<key_type, value_type>& operator=(LruCache&& _l)
    {
        m_capacity = _l.m_capacity;
        m_data = std::move(_l.m_data);
        m_index = std::move(_l.m_index);

        return *this;
    }

    size_t insert(key_type const& _key, value_type const& _val)
    {
        auto const cIter = m_index.find(_key);
        if (cIter == m_index.cend())
        {
            if (m_index.size() == m_capacity)
            {
                m_index.erase(m_data.back().first);
                m_data.pop_back();
            }
            m_data.push_front({_key, _val});
            m_index[_key] = m_data.begin();
        }
        else
        {
            m_data.splice(m_data.begin(), m_data, cIter->second);
        }

        return m_index.size();
    }

    size_t remove(key_type const& _key)
    {
        auto const cIter = m_index.find(_key);
        if (cIter != m_index.cend())
        {
            m_data.erase(cIter->second);
            m_index.erase(cIter);
        }

        return m_index.size();
    }

    bool touch(key_type const& _key)
    {
        auto const cIter = m_index.find(_key);
        if (cIter != m_index.cend())
        {
            m_data.splice(m_data.begin(), m_data, cIter->second);
            return true;
        }
        return false;
    }

    bool contains(key_type const& _key) const { return m_index.find(_key) != m_index.cend(); }

    bool contains(key_type const& _key, value_type const& _value) const
    {
        auto const cIter = m_index.find(_key);
        return cIter != m_index.cend() && (*(cIter->second)).second == _value;
    }

    bool empty() const { return m_index.empty(); }

    size_t size() const { return m_index.size(); }

    size_t capacity() const { return m_capacity; }

    void clear()
    {
        m_index.clear();
        m_data.clear();
    }

    // Expose data iterator for testing purposes
    typename list_type::const_iterator cbegin() const { return m_data.cbegin(); }
    typename list_type::iterator begin() { return m_data.begin(); }
    typename list_type::const_iterator cend() const { return m_data.cend(); }
    typename list_type::iterator end() { return m_data.end(); }

private:
    list_type m_data;
    map_type m_index;
    size_t m_capacity;
};
}  // namespace dev