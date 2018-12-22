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

/**
 * find_subsumed :  (cr : Clause*) (Vector<Clause *>& out_subsumed) ->  [void]
 * 
 * Description:
 *  Find all clauses that are subsumed by clause (*cr)
 * 
 * Reference:
 * [EB05] N.Een, A. Biere. "Effiective preprocessing in SAT through variable and
 * clause elimination". Proc. of SAT, 2005
 */
void Solver::find_subsumed(CRARef cr, Vector<CRARef>& out_subsumed) {
    Clause& c = _ca[cr];

    int min_i = 0;
    for (int i = 0; i < c.size(); i++) {
        if (_occur_lit[c[i]].size() < _occur_lit[c[min_i]].size()) {
            min_i = i;
        }
    }

    Vector<CRARef>& crs = _occur_lit[c[min_i]];
    for (int i = 0; i < crs.size(); i++) {
        Clause& cp = _ca[crs[i]];
        if (cr != crs[i] && c.subsumes(cp) == LITERAL_UNDEF) {
            out_subsumed.push(crs[i]);
        }
    }
}

/**
 * is_subsumed : (cr : Clause*) -> [bool]
 * 
 * Description:
 *  Check whether there is a clause in the database that subsumes (*cr)
 */
bool Solver::is_subsumed(CRARef cr) {
    Clause& c = _ca[cr];

    for (int i = 0; i < c.size(); i++) {
        Vector<CRARef>& crs = _occur_lit[c[i]];
        for (int j = 0; j < crs.size(); j++) {
            Clause& cp = _ca[crs[j]];
            if (cr != crs[i] && cp.subsumes(c) == LITERAL_UNDEF) {
                return true;
            }
        }
    }

    return false;
}

/**
 * reduction_subsumption : (cr : Clause*) -> [void]
 * 
 * Description:
 *  Remove all clauses that are subsumed by clause (*cr)
 */
void Solver::reduction_subsumption(CRARef cr) {
    // TODO : flag to turn off
    Vector<CRARef> subs;
    find_subsumed(cr, subs);
    for (int i = 0; i < subs.size(); i++) {
        // _subsumption_removed++;
        remove_clause(subs[i]);
    }
    
    check_garbage();
}