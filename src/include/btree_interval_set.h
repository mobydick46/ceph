// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */


#ifndef CEPH_BTREE_INTERVAL_SET_H
#define CEPH_BTREE_INTERVAL_SET_H

#include <iterator>
#include <map>
#include <ostream>

#include "encoding.h"

#ifndef MIN
# define MIN(a,b)  ((a)<=(b) ? (a):(b))
#endif
#ifndef MAX
# define MAX(a,b)  ((a)>=(b) ? (a):(b))
#endif

#include "cpp-btree/btree_map.h"
#include "assert.h"
#include "encoding_btree.h"

template<typename T,
	 typename Alloc = std::allocator<std::pair<const T, T>>>
class btree_interval_set {
 public:

  typedef btree::btree_map<T,T, std::less<T>, Alloc> map_t;

  class const_iterator;

  class iterator : public std::iterator <std::forward_iterator_tag, T>
  {
    public:
        explicit iterator(typename map_t::iterator iter)
          : _iter(iter)
        { }

        // For the copy constructor and assignment operator, the compiler-generated functions, which
        // perform simple bitwise copying, should be fine.

        bool operator==(const iterator& rhs) const {
          return (_iter == rhs._iter);
        }

        bool operator!=(const iterator& rhs) const {
          return (_iter != rhs._iter);
        }

        // Dereference this iterator to get a pair.
        std::pair < T, T > &operator*() {
                return *_iter;
        }

        // Return the interval start.
        T get_start() const {
                return _iter->first;
        }

        // Return the interval length.
        T get_len() const {
                return _iter->second;
        }

        // Set the interval length.
        void set_len(T len) {
                _iter->second = len;
        }

        // Preincrement
        iterator &operator++()
        {
                ++_iter;
                return *this;
        }

        // Postincrement
        iterator operator++(int)
        {
                iterator prev(_iter);
                ++_iter;
                return prev;
        }

    friend class btree_interval_set<T>::const_iterator;

    protected:
        typename map_t::iterator _iter;
    friend class btree_interval_set<T>;
  };

  class const_iterator : public std::iterator <std::forward_iterator_tag, T>
  {
    public:
        explicit const_iterator(typename map_t::const_iterator iter)
          : _iter(iter)
        { }

        const_iterator(const iterator &i)
	  : _iter(i._iter)
        { }

        // For the copy constructor and assignment operator, the compiler-generated functions, which
        // perform simple bitwise copying, should be fine.

        bool operator==(const const_iterator& rhs) const {
          return (_iter == rhs._iter);
        }

        bool operator!=(const const_iterator& rhs) const {
          return (_iter != rhs._iter);
        }

        // Dereference this iterator to get a pair.
        std::pair < T, T > operator*() const {
                return *_iter;
        }

        // Return the interval start.
        T get_start() const {
                return _iter->first;
        }

        // Return the interval length.
        T get_len() const {
                return _iter->second;
        }

        // Preincrement
        const_iterator &operator++()
        {
                ++_iter;
                return *this;
        }

        // Postincrement
        const_iterator operator++(int)
        {
                const_iterator prev(_iter);
                ++_iter;
                return prev;
        }

    protected:
        typename map_t::const_iterator _iter;
  };

  btree_interval_set() : _size(0) {}

  int num_intervals() const
  {
    return m.size();
  }

  typename btree_interval_set<T,Alloc>::iterator begin() {
    return typename btree_interval_set<T,Alloc>::iterator(m.begin());
  }

  typename btree_interval_set<T,Alloc>::iterator lower_bound(T start) {
    return typename btree_interval_set<T,Alloc>::iterator(find_inc_m(start));
  }

  typename btree_interval_set<T,Alloc>::iterator end() {
    return typename btree_interval_set<T,Alloc>::iterator(m.end());
  }

  typename btree_interval_set<T,Alloc>::const_iterator begin() const {
    return typename btree_interval_set<T,Alloc>::const_iterator(m.begin());
  }

  typename btree_interval_set<T,Alloc>::const_iterator lower_bound(T start) const {
    return typename btree_interval_set<T,Alloc>::const_iterator(find_inc(start));
  }

  typename btree_interval_set<T,Alloc>::const_iterator end() const {
    return typename btree_interval_set<T,Alloc>::const_iterator(m.end());
  }

  // helpers
 private:
  typename map_t::const_iterator find_inc(T start) const {
    typename map_t::const_iterator p = m.lower_bound(start);  // p->first >= start
    if (p != m.begin() &&
        (p == m.end() || p->first > start)) {
      p--;   // might overlap?
      if (p->first + p->second <= start)
        p++; // it doesn't.
    }
    return p;
  }

  typename map_t::iterator find_inc_m(T start) {
    typename map_t::iterator p = m.lower_bound(start);
    if (p != m.begin() &&
        (p == m.end() || p->first > start)) {
      p--;   // might overlap?
      if (p->first + p->second <= start)
        p++; // it doesn't.
    }
    return p;
  }

  typename map_t::const_iterator find_adj(T start) const {
    typename map_t::const_iterator p = m.lower_bound(start);
    if (p != m.begin() &&
        (p == m.end() || p->first > start)) {
      p--;   // might touch?
      if (p->first + p->second < start)
        p++; // it doesn't.
    }
    return p;
  }

  typename map_t::iterator find_adj_m(T start) {
    typename map_t::iterator p = m.lower_bound(start);
    if (p != m.begin() &&
        (p == m.end() || p->first > start)) {
      p--;   // might touch?
      if (p->first + p->second < start)
        p++; // it doesn't.
    }
    return p;
  }

 public:
  bool operator==(const btree_interval_set& other) const {
    return _size == other._size && m == other.m;
  }

  int64_t size() const {
    return _size;
  }

  void encode(bufferlist& bl) const {
    ::encode(m, bl);
  }
  void encode_nohead(bufferlist& bl) const {
    ::encode_nohead(m, bl);
  }
  void decode(bufferlist::iterator& bl) {
    ::decode(m, bl);
    _size = 0;
    for (typename map_t::const_iterator p = m.begin();
         p != m.end();
         p++)
      _size += p->second;
  }
  void decode_nohead(int n, bufferlist::iterator& bl) {
    ::decode_nohead(n, m, bl);
    _size = 0;
    for (typename map_t::const_iterator p = m.begin();
         p != m.end();
         p++)
      _size += p->second;
  }

  void clear() {
    m.clear();
    _size = 0;
  }

  bool contains(T i, T *pstart=0, T *plen=0) const {
    typename map_t::const_iterator p = find_inc(i);
    if (p == m.end()) return false;
    if (p->first > i) return false;
    if (p->first+p->second <= i) return false;
    assert(p->first <= i && p->first+p->second > i);
    if (pstart)
      *pstart = p->first;
    if (plen)
      *plen = p->second;
    return true;
  }
  bool contains(T start, T len) const {
    typename map_t::const_iterator p = find_inc(start);
    if (p == m.end()) return false;
    if (p->first > start) return false;
    if (p->first+p->second <= start) return false;
    assert(p->first <= start && p->first+p->second > start);
    if (p->first+p->second < start+len) return false;
    return true;
  }
  bool intersects(T start, T len) const {
    btree_interval_set a;
    a.insert(start, len);
    btree_interval_set i;
    i.intersection_of( *this, a );
    if (i.empty()) return false;
    return true;
  }

  // outer range of set
  bool empty() const {
    return m.empty();
  }
  T range_start() const {
    assert(!empty());
    typename map_t::const_iterator p = m.begin();
    return p->first;
  }
  T range_end() const {
    assert(!empty());
    typename map_t::const_iterator p = m.end();
    p--;
    return p->first+p->second;
  }

  // interval start after p (where p not in set)
  bool starts_after(T i) const {
    assert(!contains(i));
    typename map_t::const_iterator p = find_inc(i);
    if (p == m.end()) return false;
    return true;
  }
  T start_after(T i) const {
    assert(!contains(i));
    typename map_t::const_iterator p = find_inc(i);
    return p->first;
  }

  // interval end that contains start
  T end_after(T start) const {
    assert(contains(start));
    typename map_t::const_iterator p = find_inc(start);
    return p->first+p->second;
  }

  void insert(T val) {
    insert(val, 1);
  }

  void insert(T start, T len, T *pstart=0, T *plen=0) {
    //cout << "insert " << start << "~" << len << endl;
    assert(len > 0);
    _size += len;
    typename map_t::iterator p = find_adj_m(start);
    if (p == m.end()) {
      m[start] = len;                  // new interval
      if (pstart)
	*pstart = start;
      if (plen)
	*plen = len;
    } else {
      if (p->first < start) {

        if (p->first + p->second != start) {
          //cout << "p is " << p->first << "~" << p->second << ", start is " << start << ", len is " << len << endl;
          ceph_abort();
        }

        p->second += len;               // append to end

        typename map_t::iterator n = p;
        n++;
	if (pstart)
	  *pstart = p->first;
        if (n != m.end() &&
            start+len == n->first) {   // combine with next, too!
          p->second += n->second;
	  if (plen)
	    *plen = p->second;
          m.erase(n);
        } else {
	  if (plen)
	    *plen = p->second;
	}
      } else {
        if (start+len == p->first) {
	  if (pstart)
	    *pstart = start;
	  if (plen)
	    *plen = len + p->second;
	  T plen = p->second;
          m.erase(p);
          m[start] = len + plen;  // append to front
        } else {
          assert(p->first > start+len);
	  if (pstart)
	    *pstart = start;
	  if (plen)
	    *plen = len;
          m[start] = len;              // new interval
        }
      }
    }
  }

  void swap(btree_interval_set<T>& other) {
    m.swap(other.m);
    std::swap(_size, other._size);
  }

  void erase(iterator &i) {
    _size -= i.get_len();
    assert(_size >= 0);
    m.erase(i._iter);
  }

  void erase(T val) {
    erase(val, 1);
  }

  void erase(T start, T len) {
    typename map_t::iterator p = find_inc_m(start);

    _size -= len;
    assert(_size >= 0);

    assert(p != m.end());
    assert(p->first <= start);

    T before = start - p->first;
    assert(p->second >= before+len);
    T after = p->second - before - len;

    if (before)
      p->second = before;        // shorten bit before
    else
      m.erase(p);
    if (after)
      m[start+len] = after;
  }


  void subtract(const btree_interval_set &a) {
    for (typename map_t::const_iterator p = a.m.begin();
         p != a.m.end();
         p++)
      erase(p->first, p->second);
  }

  void insert(const btree_interval_set &a) {
    for (typename map_t::const_iterator p = a.m.begin();
         p != a.m.end();
         p++)
      insert(p->first, p->second);
  }


  void intersection_of(const btree_interval_set &a, const btree_interval_set &b) {
    assert(&a != this);
    assert(&b != this);
    clear();

    typename map_t::const_iterator pa = a.m.begin();
    typename map_t::const_iterator pb = b.m.begin();

    while (pa != a.m.end() && pb != b.m.end()) {
      // passing?
      if (pa->first + pa->second <= pb->first)
        { pa++;  continue; }
      if (pb->first + pb->second <= pa->first)
        { pb++;  continue; }
      T start = MAX(pa->first, pb->first);
      T en = MIN(pa->first+pa->second, pb->first+pb->second);
      assert(en > start);
      insert(start, en-start);
      if (pa->first+pa->second > pb->first+pb->second)
        pb++;
      else
        pa++;
    }
  }
  void intersection_of(const btree_interval_set& b) {
    btree_interval_set a;
    swap(a);
    intersection_of(a, b);
  }

  void union_of(const btree_interval_set &a, const btree_interval_set &b) {
    assert(&a != this);
    assert(&b != this);
    clear();

    //cout << "union_of" << endl;

    // a
    m = a.m;
    _size = a._size;

    // - (a*b)
    btree_interval_set ab;
    ab.intersection_of(a, b);
    subtract(ab);

    // + b
    insert(b);
    return;
  }
  void union_of(const btree_interval_set &b) {
    btree_interval_set a;
    swap(a);
    union_of(a, b);
  }

  bool subset_of(const btree_interval_set &big) const {
    for (typename map_t::const_iterator i = m.begin();
         i != m.end();
         i++)
      if (!big.contains(i->first, i->second)) return false;
    return true;
  }

  /*
   * build a subset of @other for given range [@start, @end)
   * E.g.:
   * subset_of([5~10,20~5], 0, 100) -> [5~10,20~5]
   * subset_of([5~10,20~5], 5, 25)  -> [5~10,20~5]
   * subset_of([5~10,20~5], 1, 10)  -> [5~5]
   * subset_of([5~10,20~5], 8, 24)  -> [8~7, 20~4]
   */
  void subset_of(const btree_interval_set &other, T start, T end) {
    assert(end >= start);
    clear();
    if (end == start) {
      return;
    }
    typename map_t::const_iterator p = other.find_inc(start);
    if (p == other.m.end())
      return;
    if (p->first < start) {
      if (p->first + p->second >= end) {
        insert(start, end - start);
        return;
      } else {
        insert(start, p->first + p->second - start);
        ++p;
      }
    }
    while (p != other.m.end()) {
      assert(p->first >= start);
      if (p->first >= end) {
        return;
      }
      if (p->first + p->second >= end) {
        insert(p->first, end - p->first);
        return;
      } else {
        // whole
        insert(p->first, p->second);
        ++p;
      }
    }
  }

  /*
   * build a subset of @other, starting at or after @start, and including
   * @len worth of values, skipping holes.  e.g.,
   *  span_of([5~10,20~5], 8, 5) -> [8~2,20~3]
   */
  void span_of(const btree_interval_set &other, T start, T len) {
    clear();
    typename map_t::const_iterator p = other.find_inc(start);
    if (p == other.m.end())
      return;
    if (p->first < start) {
      if (p->first + p->second < start)
	return;
      if (p->first + p->second < start + len) {
	T howmuch = p->second - (start - p->first);
	insert(start, howmuch);
	len -= howmuch;
	p++;
      } else {
	insert(start, len);
	return;
      }
    }
    while (p != other.m.end() && len > 0) {
      if (p->second < len) {
	insert(p->first, p->second);
	len -= p->second;
	p++;
      } else {
	insert(p->first, len);
	return;
      }
    }
  }

private:
  // data
  int64_t _size;
  map_t m;   // map start -> len
};


template<class T, class A>
inline std::ostream& operator<<(std::ostream& out, const btree_interval_set<T,A> &s) {
  out << "[";
  const char *prequel = "";
  for (auto i = s.begin();
       i != s.end();
       ++i)
  {
    out << prequel << i.get_start() << "~" << i.get_len();
    prequel = ",";
  }
  out << "]";
  return out;
}

template<class T,typename A>
inline void encode(const btree_interval_set<T,A>& s, bufferlist& bl)
{
  s.encode(bl);
}
template<class T,typename A>
inline void decode(btree_interval_set<T,A>& s, bufferlist::iterator& p)
{
  s.decode(p);
}

#endif
