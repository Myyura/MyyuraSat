/**
 * Literals
 */

#ifndef _MYYURASAT_LITERAL_H
#define _MYYURASAT_LITERAL_H

#include "variable.hpp"
#include "../util/intmap.hpp"
#include "../util/intset.hpp"

namespace MyyuraSat {

class Literal {
private:
    int _lit;

public:
    Literal(void) {}
    /**
     * For a variable, say x, we use 2x to represent x and 2x + 1 to represent
     * ~x respectively.
     */
    Literal(Variable var, bool sign = false) { _lit = var + var + (int)sign; }

    inline bool operator==(const Literal& p) const { return _lit == p._lit; }
    inline bool operator!=(const Literal& p) const { return _lit != p._lit; }
    inline bool operator<(const Literal& p) const { return _lit < p._lit; }
    
    inline Literal operator~(void) const { Literal q; q._lit = _lit ^ 1; return q; }
    inline Literal operator^(const bool b) const { Literal q; q._lit = _lit ^ (unsigned int)b; return q; }

    inline bool sign(void) const { return _lit & 1; }
    inline Variable variable(void) const { return _lit >> 1; }

    inline int to_int(void) const { return _lit; }
};

// some useful constants
const Literal LITERAL_UNDEF(-2);
const Literal LITERAL_ERROR(-1);

struct LiteralIndexDefault {
    Vector<Literal>::SizeType operator()(Literal l) const {
        return Vector<Literal>::SizeType(l.to_int());
    }
};

template<typename T>
using LMap = IntMap<Literal, T, LiteralIndexDefault>;

using LSet = IntSet<Literal, LiteralIndexDefault>;

}

#endif