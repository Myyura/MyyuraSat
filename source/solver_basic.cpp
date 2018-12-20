/**
 * The SAT solver
 * Basic functions
 */

#include "../include/core/solver.hpp"
#include "../include/util/algorithm.hpp"

using namespace MyyuraSat;

// Private ********************************************************************

inline bool Solver::is_removed(CRARef cr) const {
    return _ca[cr].mark() == 1;
}

inline bool Solver::is_locked(const Clause& c) const {
    // Working together with Watcher
    return value(c[0]) == LIFTED_BOOLEAN_TRUE
        && reason(c[0].variable()) != CRAREF_UNDEF
        && _ca.lea(reason(c[0].variable())) == &c;
}

bool Solver::_add_clause(Vector<Literal>& ps) {
    if (decision_level() != 0) {
        throw std::logic_error("Solver::_add_clause : decision level is not 0");
    }

    std::sort(ps.begin(), ps.end());

    // Check if clause is satisfied and remove false/duplicate literals
    // Bug here ？
    Literal p;
    int i, j;
    for (i = j = 0, p = LITERAL_UNDEF; i < ps.size(); i++) {
        if (value(ps[i]) == LIFTED_BOOLEAN_TRUE || ps[i] == ~p) {
            return true;
        } else if (value(ps[i]) != LIFTED_BOOLEAN_FALSE && ps[i] != p) {
            ps[j++] = p = ps[i];
        }
    }
    ps.shrink(i - j);

    if (ps.size() == 0) {
        return _myyura = false;
    } else if (ps.size() == 1) {
        unchecked_enqueue(ps[0]);
        return _myyura = (propagate() == CRAREF_UNDEF);
    } else {
        CRARef cr = _ca.alloc(ps, false);
        _clauses.push(cr);
        attach_clause_watcher(cr);

        // Occurence list for literal
        attach_clause_occlit(cr);
    }

    return true;
}

void Solver::remove_clause(CRARef cr) {
    Clause& c = _ca[cr];
    detach_clause_watcher(cr);

    c.mark(1);
    _ca.free(cr);
}

void Solver::reloc_all(ClauseAllocator& to) {
    // Watchers:
    // _watches.clean_all();
    // for (int v = 0; v < n_variables(); v++) {
    //     for (int s = 0; s < 2; s++) {
    //         Literal p(v, s);
    //         Vector<_Watcher>& ws = _watches[p];
    //         for (int j = 0; j < ws.size(); j++) {
    //             _ca.reloc(ws[j].cref, to);
    //         }
    //     }
    // }

    // Reasons:

    int i, j;
    // Learnts:

    // Original:

    for (i = j = 0; i < _clauses.size(); i++) {
        if (!is_removed(_clauses[i])) {
            _ca.reloc(_clauses[i], to);
            _clauses[j++] = _clauses[i];
        }
    }
    _clauses.shrink(i - j);
}

// Public *********************************************************************

// inline minor methods
// inline void Solver::check_garbage(void) {
    
// }

// inline void Solver::check_garbage(double gf) {

// }

inline int Solver::n_variables(void) const {
    return _next_variable;
}

inline bool Solver::add_clause(const Vector<Literal>& ps) {
    ps.copy_to(_add_clause_temp);
    return _add_clause(_add_clause_temp);
}

inline bool Solver::add_clause(Literal p) {
    _add_clause_temp.clear();
    _add_clause_temp.push(p);
    return _add_clause(_add_clause_temp);
}

inline bool Solver::add_clause(Literal p, Literal q) {
    _add_clause_temp.clear();
    _add_clause_temp.push(p);
    _add_clause_temp.push(q);
    return _add_clause(_add_clause_temp);
}

inline bool Solver::add_clause(Literal p, Literal q, Literal r) {
    _add_clause_temp.clear();
    _add_clause_temp.push(p);
    _add_clause_temp.push(q);
    _add_clause_temp.push(r);
    return _add_clause(_add_clause_temp);
}

inline bool Solver::add_clause(Literal p, Literal q, Literal r, Literal s) {
    _add_clause_temp.clear();
    _add_clause_temp.push(p);
    _add_clause_temp.push(q);
    _add_clause_temp.push(r);
    _add_clause_temp.push(s);
    return _add_clause(_add_clause_temp);
}

inline bool Solver::add_empty_clause(void) {
    _add_clause_temp.clear();
    return _add_clause(_add_clause_temp);
}

inline LiftedBoolean Solver::value(Variable x) const {
    return _assigns[x];
}

inline LiftedBoolean Solver::value(Literal p) const {
    return _assigns[p.variable()] ^ p.sign();
}

inline LiftedBoolean Solver::model_value(Variable x) const {
    return _model_value[x];
}

inline LiftedBoolean Solver::model_value(Literal p) const {
    return _model_value[p.variable()] ^ p.sign();
}

inline int Solver::n_assigns(void) const {
    return _trail.size();
}

inline int Solver::n_clauses(void) const {
    return _n_clauses;
}

inline bool Solver::solve(void) {
    _assumptions.clear();
    return _solve() == LIFTED_BOOLEAN_TRUE;
}

inline bool Solver::solve(Literal p) {
    _assumptions.clear();
    _assumptions.push(p);
    return _solve() == LIFTED_BOOLEAN_TRUE;
}

inline bool Solver::solve(Literal p, Literal q) {
    _assumptions.clear();
    _assumptions.push(p);
    _assumptions.push(q);
    return _solve() == LIFTED_BOOLEAN_TRUE;
}

inline bool Solver::solve(Literal p, Literal q, Literal r) {
    _assumptions.clear();
    _assumptions.push(p);
    _assumptions.push(q);
    _assumptions.push(r);
    return _solve() == LIFTED_BOOLEAN_TRUE;
}

inline void Solver::print_clauses(void) const {
    for (int i = 0; i < _clauses.size(); i++) {
        const Clause& c = _ca[_clauses[i]];
        std::cout << i << " : ";
        for (int j = 0; j < c.size(); j++) {
            std::cout << c[j].to_int() << " ";
        }
        std::cout << std::endl;
    }
}

// inline minor methods end

// major methods
Solver::Solver(void) :
    _myyura(true),
    _queue_head(0),
    _next_variable(0),
    // May not good to write like this
    _watches([&](const _Watcher& w) -> bool { return _ca[w.cref].mark() == 1; }),
    _occur_lit([&](const CRARef& cr) -> bool { return _ca[cr].mark() == 1; }) 
    {}

Solver::~Solver() {}

/**
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable 
 * will not be used as a decision variable (NOTE! This has effects on the 
 * meaning of a SATISFIABLE result).
 */
Variable Solver::new_variable(LiftedBoolean upol, bool dvar) {
    Variable v = _next_variable++;
    
    _watches.init(Literal(v, false));
    _watches.init(Literal(v, true));
    _occur_lit.init(Literal(v, false));
    _occur_lit.init(Literal(v, true));
    _touched.insert(v, true);
    _touched_list.push(v);
    _assigns.insert(v, LIFTED_BOOLEAN_UNDEF);
    _variable_info.insert(v, _VariableInfo(CRAREF_UNDEF, 0));
    _polarity.insert(v, false);
    _seen.insert(v, 0);
    _trail.reserve(v + 1);

    return v;
}

void Solver::garbage_collect(void) {
    /**
     * Initialize the next region to a size corresponding to the estimated 
     * utilization degree. This is not precise but should avoid some unnecessary 
     * reallocations for the new region:
     */
    ClauseAllocator to(_ca.size() - _ca.wasted());

    reloc_all(to);
    to.move_to(_ca);
}