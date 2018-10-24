/**
 * The SAT solver
 */

#ifndef _MYYURASAT_SOLVER_H
#define _MYYURASAT_SOLVER_H

#include "../type/variable.hpp"
#include "../type/literal.hpp"
#include "../type/lifted_boolean.hpp"
#include "../type/clause.hpp"

#include "../util/algorithm.hpp"
#include "../util/intmap.hpp"
#include "../util/intset.hpp"
#include "../util/vector.hpp"

#include <queue>
#include <stack>

namespace MyyuraSat {

class Solver {
private:
    // List of problem clauses and learnt clauses
    Vector<CRARef> _clauses;
    // Vector<CRARef> _learnts;

    // assignment stack; stores all assigments made in the order they were made
    Vector<Literal> _trail;
    // Separator indices for different decision levels in 'trail'
    Vector<int> _trail_lim;

    // Current set of assumptions provided to solve by the user
    Vector<Literal> _assumptions;

    // The current assignments
    VMap<LiftedBoolean> _assigns;

    /** 
     * Stores reason and level for each variable
     * If the current variable is a reason for a clause cr, then reason = cr.
     * Otherwise CRAREF_UNDEF.
     */
    struct _VariableInfo { 
        CRARef reason;
        int level;

        _VariableInfo() {}
        _VariableInfo(CRARef cr, int l) : reason(cr), level(l) {}
    };
    VMap<_VariableInfo> _variable_info;

    // Head of queue (as index into the trail -- no more explicit propagation queue
    int _queue_head;

    // If FALSE, the constraints are already unsatisfiable. No part of the solver 
    // state may be used!
    bool _not_unsatisfiable;

    // Next variable to be created
    Variable _next_variable;

    ClauseAllocator _ca;

    Vector<Variable> _released_variables;
    Vector<Variable> _free_variables;

    Vector<Literal> _add_clause_temp;

    // If problem is satisfiable, this vector contains the model (if any)
    Vector<LiftedBoolean> _model_value;

    /**
     * If problem is unsatisfiable (possibly under assumptions),
     * this vector represent the final conflict clause expressed in the 
     * assumptions
     */
    LSet _conflict;

    // To see if a variable had been already checked
    VMap<bool> _polarity;
    

    bool _myyura;

    // Statistics
    uint64_t _n_decision_variables, _n_clauses;

    // Main internal methods:
    // Return the next decision variable
    Literal pick_branch_literal(void);

    // Begins a new decision level
    void new_decision_level(void);

    // Enqueue a literal. Assumes value of literal is undefined
    void unchecked_enqueue(Literal p, CRARef from = CRAREF_UNDEF);

    // Test if 'p' contradicts current state, enqueue otherwise
    bool enqueue(Literal p, CRARef from = CRAREF_UNDEF);

    // Perform unit propagation. Returns possibly conflicting clause
    CRARef propagate(void);

    // Backtrack until a certain level
    void cancel_until(int level);

    // Search for a given number of conflicts
    LiftedBoolean search(int n_conflicts);

    // Main solve method
    LiftedBoolean _solve(void);

    // Operations on clauses:
    void remove_clause(CRARef cr);

    // Test if a clause has been removed
    bool is_removed(CRARef cr) const;

    // Returns TRUE if a clause is a reason for some implication in the current state
    bool is_locked(const Clause& c) const;

    // Returns LTRUE if a clause is satisfied in the current state
    LiftedBoolean is_satisfied(const Clause& c) const;

    // Misc:
    // Add a clause to the solver without making superflous internal copy
    // Will change the passed vector ps
    bool _add_clause(Vector<Literal>& ps);

    // Gives the current decisionlevel
    int decision_level(void) const;

    CRARef reason(Variable x) const;
    int level(Variable x) const;

    void reloc_all(ClauseAllocator& to);

public:
    // Constructor & Destructor
    Solver(void);
    virtual ~Solver(void);

    // Problem specification
    // Add a new variable with parameters specifying variable mode
    Variable new_variable(LiftedBoolean upol = LIFTED_BOOLEAN_UNDEF, bool dvar = true);

    // Make literal true and promise to never refer to variable again
    void release_variable(Literal l);

    // Add a clause to the solver
    bool add_clause(const Vector<Literal>& ps);
    bool add_clause(Literal p);
    bool add_clause(Literal p, Literal q);
    bool add_clause(Literal p, Literal q, Literal r);
    bool add_clause(Literal p, Literal q, Literal r, Literal s);
    bool add_empty_clause(void);

    // Solving
    // Removes already satisfied clauses
    bool simplify(void);

    // Search for a model that respects a given set of assumptions
    bool solve(const Vector<Literal>& assumps);

    // Search for a model that respects a given set of assumptions (With 
    // resource constraints).
    LiftedBoolean solve_limited(const Vector<Literal>& assumps);

    // Search for a model that respects a single assumption
    bool solve(Literal p);
    bool solve(Literal p, Literal q);
    bool solve(Literal p, Literal q, Literal r);

    // Search without assumptions
    bool solve(void);
    
    bool solve_test(void);

    // Iterate over clauses and top-level assignments
    // ClauseIterator clauses_begin(void) const;
    // ClauseIterator clauses_end(void) const;
    // LiteralIterator trail_begin(void) const;
    // LiteralIterator trail_end(void) const;

    // Read state
    // The current value of a variable
    LiftedBoolean value(Variable x) const;

    // The current value of a literal
    LiftedBoolean value(Literal p) const;

    // The value of a variable in the last model. The last call to solve must 
    // have been satisfiable.
    LiftedBoolean model_value(Variable x) const;

    // The value of a literal in the last model. The last call to solve must 
    // have been satisfiable.
    LiftedBoolean model_value(Literal p) const;

    // The current number of assigned literals
    int n_assigns(void) const;

    // The current number of orignal clauses
    int n_clauses(void) const;

    // The current number of variables
    int n_variables(void) const;
    int n_free_variables(void) const;

    // Print some current statistics to standard output
    void print_status(void) const;

    // Memory managment
    // virtual void garbage_collect(void);
    // void check_garbage(void);
    // void check_garbage(double gf);

    // Mode of operation
    // The fraction of wasted memory allowed before a garbage collection is 
    // triggered
    double garbage_frac;

    // Read instance
    void parse_dimacs(FILE* fp);
};

}

#endif