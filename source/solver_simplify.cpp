/**
 * The SAT solver
 * The Simplify Part
 */

#include "../include/core/solver.hpp"
#include "../include/util/algorithm.hpp"

using namespace MyyuraSat;

// Private

inline void Solver::touch(const Variable& x) {
    if (!_touched[x]) { 
        _touched[x] = 1;
        _touched_list.push(x);
    }
}

inline void Solver::touch(const Literal& p) {
    touch(p.variable());
}

inline void Solver::attach_clause_occlit(CRARef cr, CRARef overwrite) {
    Clause& c = _ca[cr];
    // Clause c should not be a learnt clause!

    for (int i = 0; i < c.size(); i++) {
        _occur_lit[c[i]].push(cr);
        touch(c[i]);
        if (overwrite == CRAREF_UNDEF) {
            _added.insert(cr);
        } else {
            _strengthened.insert(cr);
        }
    }
}

void Solver::find_subsumed(CRARef cr, Vector<CRARef>& out_subsumed) {
    
}


void Solver::find_subsumed(const CRARef cr, Vector<CRARef>& out_subsumed) {
    // A subsumes B: A is a subset of B

}