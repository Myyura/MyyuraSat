/**
 * OccLists -- a class for maintaining occurence lists with lazy deletion
 */

#ifndef _MYYURASAT_OCCURENCE_LIST_H
#define _MYYURASAT_OCCURENCE_LIST_H

#include "intmap.hpp"

#include <functional>

namespace MyyuraSat {

template<typename K, typename V, typename Vec, typename _Index = IntIndexDefault<K>>
class OccurenceList {
private:
    IntMap<K, Vec, _Index> _occs;
    IntMap<K, bool, _Index> _dirty;
    Vector<K> _dirties;
    std::function<bool (const V&)> _deleted;

public:
    OccurenceList(const std::function<bool (const V&)>& d, _Index index = _Index()): _occs(index), _dirty(index), _deleted(d) {}

    void init(const K& idx) {
        _occs.reserve(idx);
        _occs[idx].clear();
        _dirty.reserve(idx, 0);
    }

    // Access
    Vec& operator[](const K& idx) { return _occs[idx]; }
    Vec& lookup(const K& idx) {
        if (_dirty[idx]) { clean(idx); }
        return _occs[idx];
    }

    // Deletion
    void clean(const K& idx) {
        Vec& vec = _occs[idx];
        int i, j;
        for (i = j = 0; i < vec.size(); i++) {
            if (!_deleted(vec[i])) { vec[j++] = vec[i]; }
        }

        vec.shrink(i - j);
        _dirty[idx] = 0;
    }

    void clean_all(void) {
        for (int i = 0; i < _dirties.size(); i++) {
            // Dirties may contain duplicates so check here if a variable is already cleaned:
            if (_dirty[_dirties[i]]) { clean(_dirties[i]); }
        }

        _dirties.clear();
    }

    void smudge(const K& idx) {
        if (_dirty[idx] == 0) {
            _dirty[idx] = 1;
            _dirties.push(idx);
        }
    }

    void clear(bool free = true) {
        _occs.clear(free);
        _dirty.clear(free);
        _dirties.clear(free);
    }
};

}

#endif