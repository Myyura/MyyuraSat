/**
 * A simple set of integers
 */

#ifndef _MYYURASAT_INTSET_H
#define _MYYURASAT_INTSET_H

#include "intmap.hpp"

namespace MyyuraSat {

template<typename K, typename _Index = IntIndexDefault<K>>
class IntSet {
private:
    IntMap<K, bool, _Index> _set;
    Vector<K> _xs;

public:
    // Size operations:
    int size(void) const { return _xs.size(); }
    void clear(bool free = false) {
        if (free) { _set.clear(true); }
        else {
            for (int i = 0; i < _xs.size(); i++) { _set[_xs[i]] = false; }
            _xs.clear(free);
        }
    }

    // Allow inspecting the internal vector:
    const Vector<K>& to_vector(void) const { return _xs; }

    // Element access
    K operator[](int index) const { return _xs[index]; }

    // Modifiers:
    void insert(K k) { 
        _set.reserve(k, false);
        if(!_set[k]) {
            _set[k] = true;
            _xs.push(k);
        }
    }

    bool has(K k) {
        _set.reserve(k, false);
        return _set[k];
    }
};

}

#endif