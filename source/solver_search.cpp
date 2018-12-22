/**
 * The SAT solver
 * The Search Part
 */

#include "../include/core/solver.hpp"
#include "../include/util/algorithm.hpp"

using namespace MyyuraSat;

// Private methods
// inline minor methods
inline CRARef Solver::reason(Variable x) const {
    return _variable_info[x].reason;
}

inline int Solver::level(Variable x) const {
    return _variable_info[x].level;
}

inline bool Solver::enqueue(Literal p, CRARef from) {
    return value(p) != LIFTED_BOOLEAN_UNDEF ? 
        value(p) != LIFTED_BOOLEAN_FALSE : (unchecked_enqueue(p, from), true);
}

inline void Solver::new_decision_level(void) {
    _trail_lim.push(_trail.size());
}

inline int Solver::decision_level(void) const {
    return _trail_lim.size();
}

// major methods
void Solver::attach_clause_watcher(CRARef cr) {
    const Clause& c = _ca[cr];

    for (int i = 0; i < c.size(); i++) {
        _watches[~c[i]].push(_Watcher(cr));
    }
}

void Solver::detach_clause_watcher(CRARef cr, bool strict) {
    const Clause& c = _ca[cr];

    // strict or lazy detaching
    // TODO
}

LiftedBoolean Solver::is_satisfied(const Clause& c) const {
    LiftedBoolean result = LIFTED_BOOLEAN_FALSE;
    for (int i = 0; i < c.size(); i++) {
        if (value(c[i]) == LIFTED_BOOLEAN_TRUE) {
            return LIFTED_BOOLEAN_TRUE;
        } else if (value(c[i]) == LIFTED_BOOLEAN_UNDEF) {
            result = LIFTED_BOOLEAN_UNDEF;
        }
    }

    return result;
}

void Solver::unchecked_enqueue(Literal p, CRARef from) {
    // for (Variable i = 0; i < n_variables(); i++) {
    //     printf("decision level:%d | %d:%d\n", decision_level(), i, value(i).to_int());
    // }
    // printf("enqueue %d %d:%d %d\n", p.to_int(), p.variable(), value(p).to_int(), value(p.variable()).to_int()); 
    if (value(p) != LIFTED_BOOLEAN_UNDEF) {
        throw std::logic_error("Solver::unchecked_enqueue : p has already get an assignment!");
    }

    _assigns[p.variable()] = LiftedBoolean(!p.sign());
    _variable_info[p.variable()] = _VariableInfo(from, decision_level());
    _trail.push_lazy(p);
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
void Solver::cancel_until(int level) {
    if (decision_level() > level) {
        for (int c = _trail.size() - 1; c >= _trail_lim[level]; c--) {
            Variable x = _trail[c].variable();
            _assigns[x] = LIFTED_BOOLEAN_UNDEF;
            _polarity[x] = false;
        }
        _queue_head = _trail_lim[level];
        _trail.shrink(_trail.size() - _trail_lim[level]);
        _trail_lim.shrink(_trail_lim.size() - level);
    }
}

/**
 * Unit propagation
 * 
 * Reference:
 * [Wiki UP] https://en.wikipedia.org/wiki/Unit_propagation
 * 
 * TODO:
 * [MZ01] M.W.Moskewicz, C.F. Madigan, Y. Zhao, L. Zhang, S. Malik. "Chaff: 
 * Engineering an Efficient SAT Solver", Proc. of the 38th Design Automation
 * Conference, 2001
 */
CRARef Solver::propagate() {
    CRARef conflict = CRAREF_UNDEF;

    for (; _queue_head < _trail.size();) {
        // 'p' is enqueued fact to propagate
        Literal p = _trail[_queue_head++];
        Vector<_Watcher>& ws = _watches.lookup(p);

        for (int i = 0; i < ws.size(); i++) {
            CRARef cr = ws[i].cref;
            Clause& c = _ca[cr];
            Literal unassign_lit = LITERAL_UNDEF;
            int unassign_count = 0;
            // bool is_found = false;
            for (int j = 0; j < c.size(); j++) {
                // if (c[i].to_int() > p.to_int() && !is_found) { break; }
                
                // The clause c has already been SAT
                if (value(c[j]) == LIFTED_BOOLEAN_TRUE) { 
                    unassign_count = -1;
                    break;
                }

                if (value(c[j]) == LIFTED_BOOLEAN_UNDEF) {
                    unassign_lit = c[j];
                    unassign_count++;
                }

                // if (unassign_count == 2) { break; }
            }

            // The value of unassign_count did not be modified, which means that
            // all literals in clause c was assigned FALSE, i.e., a conflict
            if (unassign_count == 0) {
                return conflict = cr;
            }

            // Unit propagation
            if (unassign_count == 1) {
                unchecked_enqueue(unassign_lit, cr);
            }
        }
    }

    return conflict;
}

/**
 * analyze : (confl : Clause*) (out_learnt : Vector<Lit>&) (out_level : int&)  ->  [void]
 * 
 * Description:
 *  Analyze conflict and produce a reason clause.
 * 
 * Reference:
 * [MS96] J.P. Marques-Silva, K.A. Sakallah. "GRASP - A New Search Algorithm for 
 * Satisfiability", ICCAD. IEEE Computer Society Press, 1996
 */
void Solver::analyze(CRARef conflict, Vector<Literal>& out_learnt, int& out_level) {
    if (conflict == CRAREF_UNDEF) {
        throw std::logic_error("No conflict clause needs to be analyzed!");
    }

    // Generate conflict clause:
    Vector<Literal> helper_stack;
    Clause& c = _ca[conflict];
    for (int i = 0; i < c.size(); i++) {
        helper_stack.push(c[i]);
    }

    for (; !helper_stack.empty();) {
        Literal p = helper_stack.back();
        helper_stack.pop();

        if (reason(p.variable()) == CRAREF_UNDEF) {
            out_learnt.push(p);
        } else {
            Clause& d = _ca[reason(p.variable())];
            for (int i = 0; i < d.size(); i++) {
                Literal q = d[i];

                if (!_seen[q.variable()] && level(q.variable()) > 0) {
                    _seen[q.variable()] = 1;

                    if (level(q.variable()) < level(p.variable())) {
                        out_learnt.push(q);
                    } else {
                        helper_stack.push(q);
                    }
                }
            }
        }
    }

    // Find correct backtrack level:
    int max_i = 0;
    for (int i = 1; i < out_learnt.size(); i++) {
        if (level(out_learnt[i].variable()) > level(out_learnt[max_i].variable()) ) {
            max_i = i;
        }
    }
    out_level = level(out_learnt[max_i].variable());

    // Clear _seen
    for (Variable i = 0; i < n_variables(); i++) {
        _seen[i] = 0;
    }
}

/**
 * Branch on literals
 * 
 * Reference:
 * 
 * TODO:
 * [MZ01] M.W.Moskewicz, C.F. Madigan, Y. Zhao, L. Zhang, S. Malik. "Chaff: 
 * Engineering an Efficient SAT Solver", Proc. of the 38th Design Automation
 * Conference, 2001
 */
Literal Solver::pick_branch_literal(void) {
    Variable v = VARIABLE_UNDEF;
    int max_activity = 0;

    for (Variable i = 0; i < n_variables(); i++) {
        if (value(i) != LIFTED_BOOLEAN_UNDEF) { continue; }

        int activity = 0;
        for (int j = 0; j < _clauses.size(); j++) {
            Clause& c = _ca[_clauses[j]];
            if (c.mark()) { continue; }
            
            for (int k = 0; k < c.size(); k++) {
                if (c[k].variable() == i) { 
                    activity++; 
                    break;
                }
            }
        }
        if (activity > max_activity) {
            v = i;
            max_activity = activity;
        }
    }

    if (v != VARIABLE_UNDEF) {
        _polarity[v] = !_polarity[v];
        return _polarity[v] ? Literal(v) : ~Literal(v);
    }

    return LITERAL_UNDEF;
}

LiftedBoolean Solver::search(int n_conflicts) {
    if (_myyura != true) {
        throw;
    }

    Vector<Literal> learnt_clause;
    int backtrack_level;

    int sat, start = 1;

    for (; ;) {
        // Propagation
        CRARef conflict = propagate();

        sat = 1;
        LiftedBoolean stat = LIFTED_BOOLEAN_FALSE;
        for (auto& cr : _clauses) {
            Clause& c = _ca[cr];
            stat = is_satisfied(c);
            if (stat != LIFTED_BOOLEAN_TRUE) {
                sat = 0;
                break;
            }
        }

        if (sat == 1) {
            return LIFTED_BOOLEAN_TRUE;
        }

        if (start) {
            if (conflict != CRAREF_UNDEF) {
                return LIFTED_BOOLEAN_FALSE;
            }

            start = 0;
            new_decision_level();
            Literal p = pick_branch_literal();
            unchecked_enqueue(p);
        } else {
            if (stat == LIFTED_BOOLEAN_UNDEF && conflict == CRAREF_UNDEF) {
                new_decision_level();
                Literal p = pick_branch_literal();
                unchecked_enqueue(p);
            } else {
                // printf("Conflict\n");
                //     for (int i = 0; i < n_variables(); i++) {
                //         printf("%d ", _assigns[i].to_int());
                //     }
                //     printf("\n");
                // printf("Back\n");

                learnt_clause.clear();
                analyze(conflict, learnt_clause, backtrack_level);
                cancel_until(backtrack_level);

                CRARef cr = _ca.alloc(learnt_clause, true);
                _learnts.push(cr);
                attach_clause_watcher(cr);

                if (conflict != CRAREF_UNDEF) {
                    Literal p = _trail[_trail_lim[_trail_lim.size() - 1]];
                    cancel_until(decision_level() - 1);
                    new_decision_level();
                    unchecked_enqueue(p);
                }

                for (; decision_level() != 0 && _trail[_trail.size() - 1].sign();) {
                    // for (int i = 0; i < n_variables(); i++) {
                    //     printf("%d ", _assigns[i].to_int());
                    // }
                    // printf("\n");
                    cancel_until(decision_level() - 1);
                }
                // printf("Back end\n");

                if (decision_level() == 0) { 
                    return LIFTED_BOOLEAN_FALSE; 
                }

                Literal p = _trail[_trail.size() - 1];
                cancel_until(decision_level() - 1);
                new_decision_level();
                unchecked_enqueue(~p);
            }
        }
    }
}