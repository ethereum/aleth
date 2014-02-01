/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file TrieDB.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <map>
#include <memory>
#include <leveldb/db.h>
#include "TrieCommon.h"
namespace ldb = leveldb;

namespace eth
{

class DBFace
{
public:
	virtual std::string node(h256 _h) const = 0;
	virtual void insertNode(h256 _h, bytesConstRef _v) = 0;
	virtual void killNode(h256 _h) = 0;
};

class BasicMap
{
public:
	BasicMap() {}

	void clear() { m_over.clear(); }
	std::map<h256, std::string> const& get() const { return m_over; }

	std::string lookup(h256 _h) const { auto it = m_over.find(_h); if (it != m_over.end()) return it->second; return std::string(); }
	void insert(h256 _h, bytesConstRef _v) { m_over[_h] = _v.toString(); m_refCount[_h]++; }
	void kill(h256 _h) { if (!--m_refCount[_h]) m_over.erase(_h); }

protected:
	std::map<h256, std::string> m_over;
	std::map<h256, uint> m_refCount;
};

inline std::ostream& operator<<(std::ostream& _out, BasicMap const& _m)
{
	for (auto i: _m.get())
	{
		_out << i.first << ": ";
		_out << RLP(i.second);
		_out << " " << asHex(i.second);
		_out << std::endl;
	}
	return _out;
}

class Overlay: public BasicMap
{
public:
	Overlay(ldb::DB* _db = nullptr): m_db(_db) {}

	ldb::DB* db() const { return m_db.get(); }
	void setDB(ldb::DB* _db, bool _clearOverlay = true) { m_db = std::shared_ptr<ldb::DB>(_db); if (_clearOverlay) m_over.clear(); }

	void commit() { for (auto const& i: m_over) m_db->Put(m_writeOptions, ldb::Slice((char const*)i.first.data(), i.first.size), ldb::Slice(i.second.data(), i.second.size())); m_over.clear(); m_refCount.clear(); }
	void rollback() { m_over.clear(); m_refCount.clear(); }

	std::string lookup(h256 _h) const { std::string ret = BasicMap::lookup(_h); if (ret.empty()) m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret); return ret; }

private:
	using BasicMap::clear;

	std::shared_ptr<ldb::DB> m_db;

	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;
};

#if WIN32
#pragma warning(push)
#pragma warning(disable:4100) // disable warnings so it compiles
#endif

extern const h256 c_shaNull;

/**
 * @brief Merkle Patricia Tree "Trie": a modifed base-16 Radix tree.
 * This version uses an LDB backend
 */
template <class DB>
class GenericTrieDB
{
public:
	GenericTrieDB(DB* _db): m_db(_db) {}
	GenericTrieDB(DB* _db, h256 _root): m_root(_root), m_db(_db) {}
	~GenericTrieDB() {}

	void open(DB* _db, h256 _root) { m_root = _root; m_db = _db; }

	void init();
	void setRoot(h256 _root) { m_root = _root; }

	h256 root() const { return m_root == c_shaNull ? h256() : m_root; }	// patch the root in the case of the empty trie. TODO: handle this properly.

	void debugPrint() {}

	std::string at(bytesConstRef _key) const;
	void insert(bytesConstRef _key, bytesConstRef _value);
	void remove(bytesConstRef _key);

	class iterator
	{
	public:
		using value_type = std::pair<bytesConstRef, bytesConstRef>;

		iterator() {}
		iterator(GenericTrieDB const* _db)
		{
			m_that = _db;
			m_trail.push_back(Node{_db->node(_db->m_root), std::string(1, '\0'), 255});	// one null byte is the HPE for the empty key.
			next();
		}

		iterator& operator++()
		{
			next();
			return *this;
		}

		value_type operator*() const { return at(); }
		value_type operator->() const { return at(); }

		bool operator==(iterator const& _c) const { return _c.m_trail == m_trail; }
		bool operator!=(iterator const& _c) const { return _c.m_trail != m_trail; }

		value_type at() const
		{
			assert(m_trail.size());
			Node const& b = m_trail.back();
			assert(b.key.size());
			assert(!(b.key[0] & 0x10));	// should be an integer number of bytes (i.e. not an odd number of nibbles).

			RLP rlp(b.rlp);
			if (rlp.itemCount() == 2)
				return std::make_pair(bytesConstRef(b.key).cropped(1), rlp[1].payload());
			else
				return std::make_pair(bytesConstRef(b.key).cropped(1), rlp[16].payload());
		}

	private:
		void next()
		{
			while (true)
			{
				if (m_trail.empty())
				{
					m_that = nullptr;
					return;
				}

				Node const& b = m_trail.back();
				RLP rlp(b.rlp);

				if (m_trail.back().child == 255)
				{
					// Entering. Look for first...
					if (rlp.isEmpty())
					{
						m_trail.pop_back();
						continue;
					}
					assert(rlp.isList() && (rlp.itemCount() == 2 || rlp.itemCount() == 17));
					if (rlp.itemCount() == 2)
					{
						// Just turn it into a valid Branch
						m_trail.back().key = hexPrefixEncode(keyOf(m_trail.back().key), keyOf(rlp), false);
						if (isLeaf(rlp))
						{
							// leaf - exit now.
							m_trail.back().child = 0;
							return;
						}

						// enter child.
						m_trail.back().rlp = m_that->deref(rlp[1]);
						// no need to set .child as 255 - it's already done.
						continue;
					}
					else
					{
						// Already a branch - look for first valid.
						m_trail.back().setFirstChild();
						// run through to...
					}
				}
				else
				{
					// Continuing/exiting. Look for next...
					if (!(rlp.isList() && rlp.itemCount() == 17))
					{
						m_trail.pop_back();
						continue;
					}
					// else run through to...
					m_trail.back().incrementChild();
				}

				// ...here. should only get here if we're a list.
				assert(rlp.isList() && rlp.itemCount() == 17);
				for (;; m_trail.back().incrementChild())
					if (m_trail.back().child == 17)
					{
						// finished here.
						m_trail.pop_back();
						break;
					}
					else if (!rlp[m_trail.back().child].isEmpty())
					{
						if (m_trail.back().child == 16)
							return;	// have a value at this node - exit now.
						else
						{
							// lead-on to another node - enter child.
							m_trail.push_back(m_trail.back());
							m_trail.back().key = hexPrefixEncode(keyOf(m_trail.back().key), NibbleSlice(bytesConstRef(&m_trail.back().child, 1), 1), false);
							m_trail.back().rlp = m_that->deref(rlp[m_trail.back().child]);
							m_trail.back().child = 255;
							break;
						}
					}
			}
		}

		struct Node
		{
			std::string rlp;
			std::string key;		// as hexPrefixEncoding.
			byte child;				// 255 -> entering

			void setFirstChild() { child = 16; }
			void incrementChild() { child = child == 16 ? 0 : child == 15 ? 17 : (child + 1); }

			bool operator==(Node const& _c) const { return rlp == _c.rlp && key == _c.key && child == _c.child; }
			bool operator!=(Node const& _c) const { return !operator==(_c); }
		};

		std::vector<Node> m_trail;
		GenericTrieDB<DB> const* m_that;
	};

	iterator begin() const { return this; }
	iterator end() const { return iterator(); }

private:
	RLPStream& streamNode(RLPStream& _s, bytes const& _b);

	std::string atAux(RLP const& _here, NibbleSlice _key) const;

	void mergeAtAux(RLPStream& _out, RLP const& _replace, NibbleSlice _key, bytesConstRef _value);
	bytes mergeAt(RLP const& _replace, NibbleSlice _k, bytesConstRef _v);

	bool deleteAtAux(RLPStream& _out, RLP const& _replace, NibbleSlice _key);
	bytes deleteAt(RLP const& _replace, NibbleSlice _k);

	// in: null (DEL)  -- OR --  [_k, V] (DEL)
	// out: [_k, _s]
	// -- OR --
	// in: [V0, ..., V15, S16] (DEL)  AND  _k == {}
	// out: [V0, ..., V15, _s]
	bytes place(RLP const& _orig, NibbleSlice _k, bytesConstRef _s);

	// in: [K, S] (DEL)
	// out: null
	// -- OR --
	// in: [V0, ..., V15, S] (DEL)
	// out: [V0, ..., V15, null]
	bytes remove(RLP const& _orig);

	// in: [K1 & K2, V] (DEL) : nibbles(K1) == _s, 0 < _s <= nibbles(K1 & K2)
	// out: [K1, H] ; [K2, V] => H (INS)  (being  [K1, [K2, V]]  if necessary)
	bytes cleve(RLP const& _orig, uint _s);

	// in: [K1, H] (DEL) ; H <= [K2, V] (DEL)  (being  [K1, [K2, V]] (DEL)  if necessary)
	// out: [K1 & K2, V]
	bytes graft(RLP const& _orig);

	// in: [V0, ... V15, S] (DEL)
	// out1: [k{i}, Vi]    where i < 16
	// out2: [k{}, S]      where i == 16
	bytes merge(RLP const& _orig, byte _i);

	// in: [k{}, S] (DEL)
	// out: [null ** 16, S]
	// -- OR --
	// in: [k{i}, N] (DEL)
	// out: [null ** i, N, null ** (16 - i)]
	// -- OR --
	// in: [k{i}K, V] (DEL)
	// out: [null ** i, H, null ** (16 - i)] ; [K, V] => H (INS)  (being [null ** i, [K, V], null ** (16 - i)]  if necessary)
	bytes branch(RLP const& _orig);

	bool isTwoItemNode(RLP const& _n) const;
	std::string deref(RLP const& _n) const;

	std::string node(h256 _h) const { return m_db->lookup(_h); }
	void insertNode(h256 _h, bytesConstRef _v) { m_db->insert(_h, _v); }
	void killNode(h256 _h) { m_db->kill(_h); }

	h256 insertNode(bytesConstRef _v) { auto h = sha3(_v); insertNode(h, _v); return h; }
	void killNode(RLP const& _d) { if (_d.data().size() >= 32) killNode(sha3(_d.data())); }

	h256 m_root;
	DB* m_db = nullptr;
};

template <class DB>
std::ostream& operator<<(std::ostream& _out, GenericTrieDB<DB> const& _db)
{
	for (auto const& i: _db)
		_out << escaped(i.first.toString(), false) << ": " << escaped(i.second.toString(), false) << std::endl;
	return _out;
}

#if WIN32
#pragma warning(pop)
#endif

template <class KeyType, class DB>
class TrieDB: public GenericTrieDB<DB>
{
public:
	TrieDB(DB* _db): GenericTrieDB<DB>(_db) {}
	TrieDB(DB* _db, h256 _root): GenericTrieDB<DB>(_db, _root) {}

	std::string operator[](KeyType _k) const { return at(_k); }

	std::string at(KeyType _k) const { return GenericTrieDB<DB>::at(bytesConstRef((byte const*)&_k, sizeof(KeyType))); }
	void insert(KeyType _k, bytesConstRef _value) { GenericTrieDB<DB>::insert(bytesConstRef((byte const*)&_k, sizeof(KeyType)), _value); }
	void insert(KeyType _k, bytes const& _value) { insert(_k, bytesConstRef(&_value)); }
	void remove(KeyType _k) { GenericTrieDB<DB>::remove(bytesConstRef((byte const*)&_k, sizeof(KeyType))); }

	class iterator: public GenericTrieDB<DB>::iterator
	{
	public:
		using Super = typename GenericTrieDB<DB>::iterator;
		using value_type = std::pair<KeyType, bytesConstRef>;

		iterator() {}
		iterator(TrieDB const* _db): Super(_db) {}

		value_type operator*() const { return at(); }
		value_type operator->() const { return at(); }

		value_type at() const
		{
			auto p = Super::at();
			value_type ret;
			assert(p.first.size() == sizeof(KeyType));
			memcpy(&ret.first, p.first.data(), sizeof(KeyType));
			ret.second = p.second;
			return ret;
		}
	};

	iterator begin() const { return this; }
	iterator end() const { return iterator(); }
};

template <class KeyType, class DB>
std::ostream& operator<<(std::ostream& _out, TrieDB<KeyType, DB> const& _db)
{
	for (auto const& i: _db)
		_out << i.first << ": " << escaped(i.second.toString(), false) << std::endl;
	return _out;
}

}

// Template implementations...
namespace eth
{

template <class DB> void GenericTrieDB<DB>::init()
{
	m_root = insertNode(&RLPNull);
}

template <class DB> void GenericTrieDB<DB>::insert(bytesConstRef _key, bytesConstRef _value)
{
	std::string rv = node(m_root);
	bytes b = mergeAt(RLP(rv), NibbleSlice(_key), _value);

	// mergeAt won't attempt to delete the node is it's less than 32 bytes
	// However, we know it's the root node and thus always hashed.
	// So, if it's less than 32 (and thus should have been deleted but wasn't) then we delete it here.
	if (rv.size() < 32)
		killNode(m_root);
	m_root = insertNode(&b);
}

template <class DB> std::string GenericTrieDB<DB>::at(bytesConstRef _key) const
{
	return atAux(RLP(node(m_root)), _key);
}

template <class DB> std::string GenericTrieDB<DB>::atAux(RLP const& _here, NibbleSlice _key) const
{
	if (_here.isEmpty() || _here.isNull())
		// not found.
		return std::string();
	assert(_here.isList() && (_here.itemCount() == 2 || _here.itemCount() == 17));
	if (_here.itemCount() == 2)
	{
		auto k = keyOf(_here);
		if (_key == k && isLeaf(_here))
			// reached leaf and it's us
			return _here[1].toString();
		else if (_key.contains(k) && !isLeaf(_here))
			// not yet at leaf and it might yet be us. onwards...
			return atAux(_here[1].isList() ? _here[1] : RLP(node(_here[1].toHash<h256>())), _key.mid(k.size()));
		else
			// not us.
			return std::string();
	}
	else
	{
		if (_key.size() == 0)
			return _here[16].toString();
		auto n = _here[_key[0]];
		if (n.isEmpty())
			return std::string();
		else
			return atAux(n.isList() ? n : RLP(node(n.toHash<h256>())), _key.mid(1));
	}
}

template <class DB> bytes GenericTrieDB<DB>::mergeAt(RLP const& _orig, NibbleSlice _k, bytesConstRef _v)
{
//	::operator<<(std::cout << "mergeAt ", _orig) << _k << _v.toString() << std::endl;

	// The caller will make sure that the bytes are inserted properly.
	// - This might mean inserting an entry into m_over
	// We will take care to ensure that (our reference to) _orig is killed.

	// Empty - just insert here
	if (_orig.isEmpty())
		return place(_orig, _k, _v);

	assert(_orig.isList() && (_orig.itemCount() == 2 || _orig.itemCount() == 17));
	if (_orig.itemCount() == 2)
	{
		// pair...
		NibbleSlice k = keyOf(_orig);

		// exactly our node - place value in directly.
		if (k == _k && isLeaf(_orig))
			return place(_orig, _k, _v);

		// partial key is our key - move down.
		if (_k.contains(k) && !isLeaf(_orig))
		{
			killNode(sha3(_orig.data()));
			RLPStream s(2);
			s.appendRaw(_orig[0]);
			mergeAtAux(s, _orig[1], _k.mid(k.size()), _v);
			return s.out();
		}

		auto sh = _k.shared(k);
//		std::cout << _k << " sh " << k << " = " << sh << std::endl;
		if (sh)
			// shared stuff - cleve at disagreement.
			return mergeAt(RLP(cleve(_orig, sh)), _k, _v);
		else
			// nothing shared - branch
			return mergeAt(RLP(branch(_orig)), _k, _v);
	}
	else
	{
		// branch...

		// exactly our node - place value.
		if (_k.size() == 0)
			return place(_orig, _k, _v);

		// Kill the node.
		killNode(sha3(_orig.data()));

		// not exactly our node - delve to next level at the correct index.
		byte n = _k[0];
		RLPStream r(17);
		for (byte i = 0; i < 17; ++i)
			if (i == n)
				mergeAtAux(r, _orig[i], _k.mid(1), _v);
			else
				r.appendRaw(_orig[i]);
		return r.out();
	}

}

template <class DB> void GenericTrieDB<DB>::mergeAtAux(RLPStream& _out, RLP const& _orig, NibbleSlice _k, bytesConstRef _v)
{
	RLP r = _orig;
	std::string s;
	if (!r.isList() && !r.isEmpty())
	{
		s = node(_orig.toHash<h256>());
		r = RLP(s);
		assert(!r.isNull());
		killNode(_orig.toHash<h256>());
	}
	else
		killNode(_orig);
	bytes b = mergeAt(r, _k, _v);
//	::operator<<(std::cout, RLP(b)) << std::endl;
	streamNode(_out, b);
}

template <class DB> void GenericTrieDB<DB>::remove(bytesConstRef _key)
{
	std::string rv = node(m_root);
	bytes b = deleteAt(RLP(rv), NibbleSlice(_key));
	if (b.size())
	{
		if (rv.size() < 32)
			killNode(m_root);
		m_root = insertNode(&b);
	}
}

template <class DB> bool GenericTrieDB<DB>::isTwoItemNode(RLP const& _n) const
{
	return (_n.isString() && RLP(node(_n.toHash<h256>())).itemCount() == 2)
			|| (_n.isList() && _n.itemCount() == 2);
}

template <class DB> std::string GenericTrieDB<DB>::deref(RLP const& _n) const
{
	return _n.isList() ? _n.data().toString() : node(_n.toHash<h256>());
}

template <class DB> bytes GenericTrieDB<DB>::deleteAt(RLP const& _orig, NibbleSlice _k)
{
	// The caller will make sure that the bytes are inserted properly.
	// - This might mean inserting an entry into m_over
	// We will take care to ensure that (our reference to) _orig is killed.

	// Empty - not found - no change.
	if (_orig.isEmpty())
		return bytes();

	assert(_orig.isList() && (_orig.itemCount() == 2 || _orig.itemCount() == 17));
	if (_orig.itemCount() == 2)
	{
		// pair...
		NibbleSlice k = keyOf(_orig);

		// exactly our node - return null.
		if (k == _k && isLeaf(_orig))
			return RLPNull;

		// partial key is our key - move down.
		if (_k.contains(k))
		{
			RLPStream s;
			s.appendList(2) << _orig[0];
			if (!deleteAtAux(s, _orig[1], _k.mid(k.size())))
				return bytes();
			killNode(sha3(_orig.data()));
			RLP r(s.out());
			if (isTwoItemNode(r[1]))
				return graft(r);
			return s.out();
		}
		else
			// not found - no change.
			return bytes();
	}
	else
	{
		// branch...

		// exactly our node - remove and rejig.
		if (_k.size() == 0 && !_orig[16].isEmpty())
		{
			// Kill the node.
			killNode(sha3(_orig.data()));

			byte used = uniqueInUse(_orig, 16);
			if (used != 255)
				if (_orig[used].isList() && _orig[used].itemCount() == 2)
					return graft(RLP(merge(_orig, used)));
				else
					return merge(_orig, used);
			else
			{
				RLPStream r(17);
				for (byte i = 0; i < 16; ++i)
					r << _orig[i];
				r << "";
				return r.out();
			}
		}
		else
		{
			// not exactly our node - delve to next level at the correct index.
			RLPStream r(17);
			byte n = _k[0];
			for (byte i = 0; i < 17; ++i)
				if (i == n)
					if (!deleteAtAux(r, _orig[i], _k.mid(1)))	// bomb out if the key didn't turn up.
						return bytes();
					else {}
				else
					r << _orig[i];

			// check if we ended up leaving the node invalid.
			RLP rlp(r.out());
			byte used = uniqueInUse(rlp, 255);
			if (used == 255)	// no - all ok.
				return r.out();

			// yes; merge
			if (isTwoItemNode(rlp[used]))
				return graft(RLP(merge(rlp, used)));
			else
				return merge(rlp, used);
		}
	}

}

template <class DB> bool GenericTrieDB<DB>::deleteAtAux(RLPStream& _out, RLP const& _orig, NibbleSlice _k)
{
	bytes b = deleteAt(_orig.isList() ? _orig : RLP(node(_orig.toHash<h256>())), _k);

	if (!b.size())	// not found - no change.
		return false;

	if (_orig.isList())
		killNode(_orig);
	else
		killNode(_orig.toHash<h256>());

	streamNode(_out, b);
	return true;
}

template <class DB> bytes GenericTrieDB<DB>::place(RLP const& _orig, NibbleSlice _k, bytesConstRef _s)
{
//	::operator<<(std::cout << "place ", _orig) << ", " << _k << ", " << _s.toString() << std::endl;

	killNode(_orig);
	if (_orig.isEmpty())
		return RLPStream(2).appendString(hexPrefixEncode(_k, true)).appendString(_s).out();

	assert(_orig.isList() && (_orig.itemCount() == 2 || _orig.itemCount() == 17));
	if (_orig.itemCount() == 2)
		return RLPStream(2).appendRaw(_orig[0]).appendString(_s).out();

	auto s = RLPStream(17);
	for (uint i = 0; i < 16; ++i)
		s.appendRaw(_orig[i]);
	s.appendString(_s);
	return s.out();
}

// in1: [K, S] (DEL)
// out1: null
// in2: [V0, ..., V15, S] (DEL)
// out2: [V0, ..., V15, null] iff exists i: !!Vi  -- OR --  null otherwise
template <class DB> bytes GenericTrieDB<DB>::remove(RLP const& _orig)
{
	killNode(_orig);

	assert(_orig.isList() && (_orig.itemCount() == 2 || _orig.itemCount() == 17));
	if (_orig.itemCount() == 2)
		return RLPNull;
	RLPStream r(17);
	for (uint i = 0; i < 16; ++i)
		r.appendRaw(_orig[i]);
	r.appendString("");
	return r.out();
}

template <class DB> RLPStream& GenericTrieDB<DB>::streamNode(RLPStream& _s, bytes const& _b)
{
	if (_b.size() < 32)
		_s.appendRaw(_b);
	else
		_s.append(insertNode(&_b));
	return _s;
}

template <class DB> bytes GenericTrieDB<DB>::cleve(RLP const& _orig, uint _s)
{
//	::operator<<(std::cout << "cleve ", _orig) << ", " << _s << std::endl;

	killNode(_orig);
	assert(_orig.isList() && _orig.itemCount() == 2);
	auto k = keyOf(_orig);
	assert(_s && _s <= k.size());

	RLPStream bottom(2);
	bottom.appendString(hexPrefixEncode(k, isLeaf(_orig), _s));
	bottom.appendRaw(_orig[1]);

	RLPStream top(2);
	top.appendString(hexPrefixEncode(k, false, 0, _s));
	streamNode(top, bottom.out());

	return top.out();
}

template <class DB> bytes GenericTrieDB<DB>::graft(RLP const& _orig)
{
	assert(_orig.isList() && _orig.itemCount() == 2);
	std::string s;
	RLP n;
	if (_orig[1].isList())
		n = _orig[1];
	else
	{
		// remove second item from the trie after derefrencing it into s & n.
		auto lh = _orig[1].toHash<h256>();
		s = node(lh);
		killNode(lh);
		n = RLP(s);
	}
	assert(n.itemCount() == 2);

	return (RLPStream(2) << hexPrefixEncode(keyOf(_orig), keyOf(n), isLeaf(n)) << n[1]).out();
//	auto ret =
//	std::cout << keyOf(_orig) << " ++ " << keyOf(n) << " == " << keyOf(RLP(ret)) << std::endl;
//	return ret;
}

template <class DB> bytes GenericTrieDB<DB>::merge(RLP const& _orig, byte _i)
{
	assert(_orig.isList() && _orig.itemCount() == 17);
	RLPStream s(2);
	if (_i != 16)
	{
		assert(!_orig[_i].isEmpty());
		s << hexPrefixEncode(bytesConstRef(&_i, 1), false, 1, 2, 0);
	}
	else
		s << hexPrefixEncode(bytes(), true);
	s << _orig[_i];
	return s.out();
}

template <class DB> bytes GenericTrieDB<DB>::branch(RLP const& _orig)
{
//	::operator<<(std::cout << "branch ", _orig) << std::endl;

	assert(_orig.isList() && _orig.itemCount() == 2);

	auto k = keyOf(_orig);
	RLPStream r(17);
	if (k.size() == 0)
	{
		assert(isLeaf(_orig));
		for (uint i = 0; i < 16; ++i)
			r << "";
		r << _orig[1];
	}
	else
	{
		byte b = k[0];
		for (uint i = 0; i < 16; ++i)
			if (i == b)
				if (isLeaf(_orig) || k.size() > 1)
				{
					RLPStream bottom(2);
					bottom.appendString(hexPrefixEncode(k.mid(1), isLeaf(_orig)));
					bottom.appendRaw(_orig[1]);
					streamNode(r, bottom.out());
				}
				else
					r << _orig[1];
			else
				r << "";
		r << "";
	}
	return r.out();
}

}
