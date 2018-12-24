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

void Solver::attach_clause_occlit(CRARef cr, CRARef overwrite) {
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

void Solver::detach_clause_occlit(CRARef cr, bool strict) {
    const Clause& c = _ca[cr];

    // strict or lazy detaching
    if (strict) {
        for (int i = 0; i < c.size(); i++) {
            remove(_occur_lit[c[i]], cr);
            touch(c[i]);
        }
    } else {
        for (int i = 0; i < c.size(); i++) {
            _occur_lit[c[i]];
            touch(c[i]);
        }
    }

    _added.erase(cr);
    _strengthened.erase(cr);
}

/**
 * toplevel_remove_clause : (cs : Vector<Clause*>&) -> [void]
 * 
 * Description:
 *  Remove all clauses (in cs) that are already satisfied in the toplevel.
 *  Remove all literals that are assigned to FALSE (in the toplevel) in an 
 *  unsatisfied clause (in cs).
 *  
 * Post-condition:
 *  Omitted
 */
void Solver::toplevel_simplify_satisfied_clause(Vector<CRARef>& cs) {
    if (decision_level() != 0) {
        throw std::logic_error("Solver::toplevel_simplify_satisfied_clause : we are not in the toplevel!");
    }

    int i, j;
    for (i = j = 0; i < cs.size(); i++) {
        Clause& c = _ca[cs[i]];
        if (is_satisfied(c) == LIFTED_BOOLEAN_TRUE) {
            remove_clause(cs[i]);
        } else {
            // TODO: trim clause
        }
    }

    // cs.shrink(i - j);
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
            if (is_removed(crs[j])) { continue; }

            Clause& cp = _ca[crs[j]];
            if (cr != crs[i] && cp.subsumes(c) == LITERAL_UNDEF) {
                return true;
            }
        }
    }

    return false;
}

/**
 * subsume0 : (cr : Clause*) -> [void]
 * 
 * Description:
 *  Remove all clauses that are subsumed by clause (*cr)
 */
void Solver::subsume0(CRARef cr) {
    // TODO : flag to turn off
    Clause& c = _ca[cr];

    int min_i = 0;
    for (int i = 0; i < c.size(); i++) {
        if (_occur_lit[c[i]].size() < _occur_lit[c[min_i]].size()) {
            min_i = i;
        }
    }

    Vector<CRARef>& crs = _occur_lit[c[min_i]];
    for (int i = 0; i < crs.size(); i++) {
        if (is_removed(crs[i])) { continue; }

        Clause& cp = _ca[crs[i]];
        if (cr != crs[i] && c.subsumes(cp) == LITERAL_UNDEF) {
            // _subsumption_removed++;
            remove_clause(crs[i]);
        }
    }
    
    check_garbage();
}

/**
 * subsume1 : (cr : Clause*) -> [void]
 * 
 * Description:
 *  Strengthen all clauses that are selfsubsumed by clause (*cr)
 */
void Solver::subsume1(CRARef cr) {
    // TODO : flag to turn off
    Vector<CRARef> subs_queue;
    int q;
    for (q = 0, subs_queue.push(cr); q < subs_queue.size(); q++) {
        if (is_removed(subs_queue[q])) { q++; continue; }
        Clause& c = _ca[subs_queue[q]];

        int min_i = 0;
        for (int i = 0; i < c.size(); i++) {
            if (_occur_lit[c[i]].size() < _occur_lit[c[min_i]].size()) {
                min_i = i;
            }
        }

        Vector<CRARef>& crs = _occur_lit[c[min_i]];
        for (int i = 0; i < crs.size(); i++) {
            if (is_removed(crs[i])) { continue; }

            if (cr != crs[i]) {
                Clause& cp = _ca[crs[i]];
                Literal p = c.subsumes(cp);
                if (p != LITERAL_UNDEF && p != LITERAL_ERROR) {
                    // _selfsubsumption_removed++;
                    cp.strengthen(p);
                    subs_queue.push(crs[i]);
                }
            }
        }
    }

    check_garbage();
}

/**
 * reduction_by_subsumption : (void) -> [void]
 * 
 * Description:
 *  Simlify the clause database by subsumption (subsume0) and selfsubsumption
 *  (subsume1).
 * 
 * Post-conditions:
 *  (1) No opportunities remain for subsumption or selfsubsumption
 *  (2) The two sets _added and _strengthened are empty
 */
void Solver::reduction_by_subsumption(void) {
    if (decision_level() != 0) {
        throw std::logic_error("Solver::reduction_by_subsumption : we are not in the toplevel!");
    }
}