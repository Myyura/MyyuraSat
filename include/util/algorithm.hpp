/**
 * Make life easier
 */

#ifndef _MYYURASAT_ALGORITHM_H
#define _MYYURASAT_ALGORITHM_H

#include "vector.hpp"

namespace MyyuraSat {

/**
 * Removing and searching for elements:
 */

template<typename V, typename T>
static inline void remove(V& ts, const T& t) {
    int i = 0;
    for (; i < (int)ts.size() && ts[i] != t; i++) {}
    for (; i < (int)ts.size() - 1; i++) { ts[i] = ts[i + 1]; }
    ts.pop();
}

template<typename V, typename T>
static inline bool find(V& ts, const T& t) {
    int i = 0;
    for (; i < (int)ts.size() && ts[i] != t; i++) {}
    return i < (int)ts.size();
}

}




#endif