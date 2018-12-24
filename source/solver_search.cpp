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

    _watches[~c[0]].push(_Watcher(cr, c[1]));
    _watches[~c[1]].push(_Watcher(cr, c[0]));

    // for (int i = 0; i < c.size(); i++) {
    //     _watches[~c[i]].push(_Watcher(cr));
    // }
}

void Solver::detach_clause_watcher(CRARef cr, bool strict) {
    const Clause& c = _ca[cr];

    // strict or lazy detaching
    if (strict) {
        remove(_watches[~c[0]], _Watcher(cr, c[1]));
        remove(_watches[~c[1]], _Watcher(cr, c[0]));
    } else {
        _watches.smudge(~c[0]);
        _watches.smudge(~c[1]);
    }
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
    std::cout << "propagate begin! =================" << std::endl;
    CRARef conflict = CRAREF_UNDEF;

    for (; _queue_head < _trail.size();) {
        // 'p' is enqueued fact to propagate
        Literal p = _trail[_queue_head++];
        Vector<_Watcher>& ws = _watches.lookup(p);
        Vector<_Watcher>::Iterator i, j;

        for (i = j = ws.begin(); i != ws.end();) {
            // Try to avoid inspecting the clause:
            Literal blocker = (*i).blocker;
            if (value(blocker) == LIFTED_BOOLEAN_TRUE) {
                *j++ = *i++;
                continue;
            }

            CRARef cr = (*i).cref;
            // std::cout << "cr: " << cr << " | size: " << _ca.size() << std::endl;
            Clause& c = _ca[cr];

            // Make sure the false literal is _data[1]:
            Literal false_lit = ~p;
            if (c[0] == false_lit) {
                c[0] = c[1];
                c[1] = false_lit;
            }
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Literal first = c[0];
            _Watcher w(cr, first);
            if (first != blocker && value(first) == LIFTED_BOOLEAN_TRUE) {
                *j++ = w; 
                continue; 
            }

            // Look for new watch:
            bool found_watch = false;
            for (int k = 2; k < c.size(); k++) {
                if (value(c[k]) != LIFTED_BOOLEAN_FALSE) {
                    c[1] = c[k];
                    c[k] = false_lit;
                    _watches[~c[1]].push(w);
                    found_watch = true;
                    break;
                }
            }

            if (!found_watch) {
                // Did not find watch -- clause is unit under assignment:
                *j++ = w;
                if (value(first) == LIFTED_BOOLEAN_FALSE){
                    conflict = cr;
                    _queue_head = _trail.size();
                    // Copy the remaining watches:
                    while (i != ws.end()) {
                        *j++ = *i++;
                    }
                } else {
                    unchecked_enqueue(first, cr);
                }
            }
        }

        ws.shrink(i - j);
    }
    std::cout << "propagate end! =================" << std::endl;
    return conflict;
}

/**
 * analyze : (conflict : Clause*) (out_learnt : Vector<Literal>&) (out_level : int&)  ->  [void]
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
    std::cout << "analyze begin=============" << std::endl;
    // Generate conflict clause:
    int path_conflict = 0;
    Literal p = LITERAL_UNDEF;

    // (leave room for the asserting literal)
    out_learnt.push();
    int index = _trail.size() - 1;

    do {
        std::cout << path_conflict << std::endl;
        if (conflict == CRAREF_UNDEF) {
            throw std::logic_error("Solver::analyze : no conflict to analyze!");
        }
        
        Clause& c = _ca[conflict];

        for (int j = (p == LITERAL_UNDEF) ? 0 : 1; j < c.size(); j++){
            Literal q = c[j];

            if (!_seen[q.variable()] && level(q.variable()) > 0){
                _seen[q.variable()] = 1;
                if (level(q.variable()) >= decision_level())
                    path_conflict++;
                else
                    out_learnt.push(q);
            }
        }
        
        // Select next clause to look at:
        while (!_seen[_trail[index--].variable()]);
        p = _trail[index + 1];
        conflict = reason(p.variable());
        _seen[p.variable()] = 0;
        path_conflict--;

    } while (path_conflict > 0);
    out_learnt[0] = ~p;

    // Find correct backtrack level:
    if (out_learnt.size() == 1) {
        out_level = 0;
    } else {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++) {
            if (level(out_learnt[i].variable()) > 
                level(out_learnt[max_i].variable())) {
                max_i = i;
            }
        }
        // Swap-in this literal at index 1:
        Literal p = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = p;
        out_level = level(p.variable());
    }

    // Clear _seen
    for (Variable i = 0; i < n_variables(); i++) {
        _seen[i] = 0;
    }

    std::cout << "analyze end==========================" << std::endl;
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

    for (; ;) {
        // Propagation
        CRARef conflict = propagate();
        std::cout << conflict << std::endl;
        if (conflict != CRAREF_UNDEF) {
            if (decision_level() == 0) { return LIFTED_BOOLEAN_FALSE; }

            learnt_clause.clear();
            analyze(conflict, learnt_clause, backtrack_level);
            cancel_until(backtrack_level);

            if (learnt_clause.size() == 1) {
                unchecked_enqueue(learnt_clause[0]);
            } else {
                CRARef cr = _ca.alloc(learnt_clause, true);
                _learnts.push(cr);
                attach_clause_watcher(cr);
                unchecked_enqueue(learnt_clause[0], cr);
            }
        } else {
            // NO CONFLICT
            Literal next = LITERAL_UNDEF;

            if (next == LITERAL_UNDEF){
                // New variable decision:
                next = pick_branch_literal();

                if (next == LITERAL_UNDEF)
                    // Model found:
                    return LIFTED_BOOLEAN_TRUE;
            }

            // Increase decision level and enqueue 'next'
            new_decision_level();
            unchecked_enqueue(next);
        }
    }
}