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
#include "../util/occurence_list.hpp"

#include <queue>
#include <stack>
#include <functional>
#include <iostream>

namespace MyyuraSat {

class Solver {
private:
    // List of problem clauses and learnt clauses
    Vector<CRARef> _clauses;
    Vector<CRARef> _learnts;

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

    // '_watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true)
    struct _Watcher {
        CRARef cref;
        Literal blocker;
        _Watcher(CRARef cr, Literal p): cref(cr), blocker(p) {}

        bool operator==(const _Watcher& w) const { return cref == w.cref; }
        bool operator!=(const _Watcher& w) const { return cref != w.cref; }
    };

    // std::function<bool (const _Watcher&)> _watcher_deleted 
    //     = [&](const _Watcher& w) -> bool { return _ca[w.cref].mark() == 1; };

    // struct _WatcherDeleted {
    //     const ClauseAllocator& ca;

    //     _WatcherDeleted(const ClauseAllocator& caca): ca(caca) {}
    //     bool operator()(const _Watcher& w) const { return ca[w.cref].mark() == 1; }
    // };
    
    OccurenceList<Literal, _Watcher, Vector<_Watcher>, LiteralIndexDefault> _watches;

        // Attach and detach a clause to watcher lists
    void attach_clause_watcher(CRARef cr);
    void detach_clause_watcher(CRARef cr, bool strict = false);

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

    // GRASP
    void analyze(CRARef conflict, Vector<Literal>& out_learnt, int& out_level);

    // Search for a given number of conflicts
    LiftedBoolean search(int n_conflicts);

    // Main solve method
    LiftedBoolean _solve(void);

    // Remove a clause from data set
    void remove_clause(CRARef cr);

    // Test if a clause has been removed
    bool is_removed(CRARef cr) const;

    // Returns TRUE if a clause is a reason for some implication in the current state
    bool is_locked(const Clause& c) const;

    // Returns LTRUE if a clause is satisfied in the current state
    LiftedBoolean is_satisfied(const Clause& c) const;

    // Add a clause to the solver without making superflous internal copy
    // Will change the passed vector ps
    bool _add_clause(Vector<Literal>& ps);

    // Gives the current decisionlevel
    int decision_level(void) const;

    CRARef reason(Variable x) const;
    int level(Variable x) const;

    // Only used in toplevel
    void toplevel_simplify_satisfied_clause(Vector<CRARef>& cs);

    /**
     * Subsumption:
     * 
     * '_occur[lit]' - a list of constraints containing 'lit'
     * 'touched' - Set to true when a variable is touched. Also true initially
     * 'touched_list' - A list of the true elements in 'touched'
     * 'added' : Clauses created
     * 'strengthened' : Clauses strengthened
     */
    OccurenceList<Literal, CRARef, Vector<CRARef>, LiteralIndexDefault> _occur_lit;

    void attach_clause_occlit(CRARef cr, CRARef overwrite = CRAREF_UNDEF);
    void detach_clause_occlit(CRARef cr, bool strict = false);

    VMap<bool> _touched;
    Vector<Variable> _touched_list;
    CSet _added;
    CSet _strengthened;

    bool is_subsumed(CRARef cr);
    void subsume0(CRARef cr);
    void subsume1(CRARef cr);
    void touch(const Variable& x);
    void touch(const Literal& p);
    void reduction_by_subsumption(void);

    /**
     * Temporaries (to reduce allocation overhead)
     */
    VMap<char> _seen;
    Vector<Literal> _add_clause_temp;

    /**
     * Garbage collection:
     * 
     *  _garbage_frac - The fraction of wasted memory allowed before a garbage 
     * collection is triggered
     */
    void reloc_all(ClauseAllocator& to);
    virtual void garbage_collect(void);
    double _garbage_frac;

public:
    // Constructor & Destructor
    Solver(void);
    virtual ~Solver(void);

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

    // Print the current set of clauses and trail of assignments, mainly for tests
    void print_clauses(void) const;
    void print_assignments(void) const;

    /**
     * Memory managment
     */
    void check_garbage(void);
    void check_garbage(double gf);

    // Mode of operation

    // Only for debugging
    bool solve_test(void);
    void clause_test(void);
    void garbage_collection_test(void);
    void subsumption_test(void);
};

}

#endif