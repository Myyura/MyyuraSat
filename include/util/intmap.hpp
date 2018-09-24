/**
 * A simple map : int -> other
 */

#ifndef _MYYURASAT_INTMAP_H
#define _MYYURASAT_INTMAP_H

#include "vector.hpp"

namespace MyyuraSat {

template<typename K, typename V, typename _Index = IntIndexDefault<K>>
class IntMap {
private:
    Vector<V> _map;
    _Index _index;

public:
    // Constructor:
    explicit IntMap(_Index index = _Index()): _index(index) {}

    // Element access
    bool has(K k) const { return _index(k) < _map.size(); }

    const V& operator[](K k) const {
        if (!has(k)) { throw std::out_of_range("IntMap<K, V>::operator[] : out of range"); }

        return _map[_index(k)];
    }

    V& operator[](K k) {
        if (!has(k)) { throw std::out_of_range("IntMap<K, V>::operator[] : out of range"); }

        return _map[_index(k)];
    }

    // Iterators
    using Iterator = V*;
    using ConstIterator = const V*;

    Iterator begin(void) { return &_map[0]; }
    ConstIterator cbegin(void) const { return &_map[0]; }
    Iterator end(void) { return &_map[_map.size()]; }
    ConstIterator cend(void) const { return &_map[_map.size()]; }

    // Size operations:
    void reserve(K key, V pad) { _map.grow_to(_index(key) + 1, pad); }

    void reserve(K key) { _map.grow_to(_index(key) + 1); }

    void clear(bool dispose = false) { _map.clear(dispose); }

    // Modifiers:
    void insert(K key, V val, V pad) {
        reserve(key, pad);
        operator[](key) = val;
    }

    void insert(K key, V val) {
        reserve(key);
        operator[](key) = val;
    }

    // Duplicatation
    void move_to(IntMap& to) {
        _map.move_to(to._map);
        to._index = _index;
    }

    void copy_to(IntMap& to) const {
        _map.copy_to(to._map);
        to._index = _index;
    }
};

};

#endif