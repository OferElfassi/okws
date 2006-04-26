
// -*-c++-*-
/* $Id$ */

#include "ihash.h"
#include "qhash.h"
#include "list.h"

#ifndef _PUB_TIMEHASH_H
#define _PUB_TIMEHASH_H

template<class K, class V, class F>
class timehash_node_t {
public:
  timehash_node_t (const V &v) : _val (v), _ctime (timenow) {}

  V _val;
  time_t _ctime;

  ihash_entry<timehash_node_t<K,V,F> > _hlnk;
  tailq_entry<timehash_node_t<K,V,F> > _qlnk;
};

template<class K, class V, class F> 
struct keyfn<timehash_node_t<K,V,F>, K> {
  keyfn () {}
  K operator() (const timehash_node_t<K,V,F> *n) 
    const { return _fn (&n->_val); }
  const F _fn;
};

template<class K, class V, class F>
void node2key (typename callback<void, K>::ref cb, timehash_node_t<K,V,F> *n)
{
  const F _fn;
  (*cb) (_fn (&n->_val));
}

template<class K, class V, class F = keyfn<V,K> >
class timehash_t 
{
public:
  timehash_t (u_int to = 0, bool rj = false) 
    : _timeout (to), _rejuvenate (rj) {}
  typedef timehash_node_t<K,V,F> node_t ;

  void set_timeout (u_int t) { _timeout = t; }
  
  void insert (const V &v) 
  {
    const K k (_fn (&v));
    remove (k);

    node_t *n = New node_t (v);
    _h.insert (n);
    _q.insert_tail (n);
  }

  V *operator[] (const K &k) 
  {
    node_t *n;
    if (_rejuvenate) {
      if ((n = _h[k])) 
	rejuvenate (n);
      expire ();
    }  else {
      expire ();
      n = _h[k];
    }
    expire ();
    if (n) { return &n->_val; }
    else { return NULL; }
  }

  bool remove (const K &k)
  {
    expire ();
    node_t *n = _h[k];
    bool ret = false;
    if (n) {
      remove (n);
      ret = true;
    } 
    return ret;
  }

  void rejuvenate (node_t *n)
  {
    _q.remove (n);
    n->_ctime = timenow;
    _q.insert_tail (n);
  }

  bool rejuvenate (const K &k)
  {
    node_t *n = _h[k];
    bool ret = false;
    if (n) {
      rejuvenate (n);
      ret = true;
    }
    return ret;
  }

  void remove (node_t *n)
  {
    _h.remove (n);
    _q.remove (n);
    delete n;
  }

  void expire ()
  {
    if (_timeout)
      expire (timenow - _timeout);
  }

  void expire (time_t deadline)
  {
    node_t *nn;
    for (node_t *n = _q.first; n; n = nn) {
      nn = _q.next (n);
      if (n->_ctime < deadline)
	remove (n);
      else
	break;
    }
  }

  void clear ()
  {
    for (node_t *n = _q.first; n ; n = _q.next (n)) {
      remove (n);
    }
  }

  void traverse_keys (typename callback<void, K>::ref cb) 
  {
    expire ();
    _q.traverse (wrap (node2key<K,V,F>, cb));
  }

  size_t size () const { return _h.size (); }

private:
  tailq<node_t, &node_t::_qlnk> _q;
  fhash<K, node_t, &node_t::_hlnk> _h;
  const F _fn;
  u_int _timeout;
  bool _rejuvenate;
};

template<> struct keyfn<str,str> {
  keyfn () {}
  const str &operator() (const str *s) const { return *s; }
};


#endif /* _PUB_TIMEHASH_H */
