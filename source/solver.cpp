/**
 * The SAT solver
 */

#include "../include/core/solver.hpp"
#include "../include/util/algorithm.hpp"

using namespace MyyuraSat;

/**
 * Parse dimacs files
 */
class DimacsReader {
private:
    FILE *_in;
    char* _current_line;
    int _pos;
    int _size;

    void skip_whitespace(void) {
        for (; (_current_line[_pos] >= 9 
            && _current_line[_pos] <= 13)
            || _current_line[_pos] == 32; _pos++) {}
    }

    int parse_integer(void) {
        int value = 0;
        bool neg = false;
        skip_whitespace();

        if (_current_line[_pos] == '-') {
            neg = true;
            _pos++;
        } else if (_current_line[_pos] == '+') {
            _pos++;
        }

        if (_current_line[_pos] < '0' || _current_line[_pos] > '9') {
            fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", _current_line[_pos]);
            std::exit(3);
        }

        for (; _current_line[_pos] >= '0' && _current_line[_pos] <= '9'; _pos++) {
            value = value * 10 + (_current_line[_pos] - '0');
        }

        return neg ? -value : value;
    }

    bool match(const char *s) {
        for (; *s != '\0'; s++, _pos++) {
            if (*s != _current_line[_pos]) {
                return false;
            }
        }

        return true;
    }

    bool read_line(void) {
        if (fgets(_current_line, _size, _in) == NULL) {
            return false;
        }

        _pos = 0;
        return true;
    }

    void read_clause(Solver& s, Vector<Literal>& lits) {
        int parsed_lit, var;
        lits.clear();
        for (; ;) {
            parsed_lit = parse_integer();
            if (parsed_lit == 0) {
                break;
            }
            var = std::abs(parsed_lit) - 1;

            for (; var >= s.n_variables();) {
                s.new_variable();
            }

            lits.push((parsed_lit > 0) ? Literal(var) : ~Literal(var));
        }
    }

public:
    explicit DimacsReader(FILE *input, int line_size = 1024) : _in(input), _pos(0), _size(line_size) {
        _current_line = (char*)std::realloc(NULL, _size);
        if (_current_line == NULL) { throw std::bad_alloc(); }
    }

    ~DimacsReader(void) { std::free(_current_line); }

    void parse_dimacs(Solver& s) {
        Vector<Literal, int> lits;
        int vars = 0;
        int clauses = 0;
        int count = 0;

        for (; read_line();) {
            skip_whitespace();
            if (_current_line[_pos] == '\0') {
                continue;
            }

            if (_current_line[_pos] == 'p') {
                if (match("p cnf")) {
                    vars = parse_integer();
                    clauses = parse_integer();
                } else {
                    fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", _current_line[_pos]);
                    exit(3);
                }
            } else if (_current_line[_pos] == 'c' || _current_line[_pos] == 'p') {
                continue;
            } else {
                count++;
                read_clause(s, lits);
                s.add_clause(lits);
            }
        }

        if (count != clauses) {
            fprintf(stderr, "PARSE ERROR! DIMACS header mismatch: wrong number of clauses\n");
        }
    }
};


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

inline bool Solver::is_removed(CRARef cr) const {
    return _ca[cr].mark() == 1;
}

inline bool Solver::is_locked(const Clause& c) const {
    return value(c[0]) == LIFTED_BOOLEAN_TRUE
        && reason(c[0].variable()) != CRAREF_UNDEF
        && _ca.lea(reason(c[0].variable())) == &c;
}

inline void Solver::new_decision_level(void) {
    _trail_lim.push(_trail.size());
}

inline int Solver::decision_level(void) const {
    return _trail_lim.size();
}

// major methods
bool Solver::_add_clause(Vector<Literal>& ps) {
    if (decision_level() != 0) {
        throw std::logic_error("Solver::_add_clause : decision level is not 0");
    }

    // Check if clause is satisfied and remove false/duplicate literals
    std::sort(ps.begin(), ps.end());

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
    } else if (ps.size() == 1 && false) {
        // TODO : 
        // ... unchecked_enqueue(ps[0]);
    } else {
        CRARef cr = _ca.alloc(ps, false);
        _clauses.push(cr);
    }

    return true;
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
    for (Variable i = 0; i < n_variables(); i++) {
        printf("%d:%d\n", i, value(i).to_int());
    }
    printf("enqueue %d %d:%d %d\n", p.to_int(), p.variable(), value(p).to_int(), value(p.variable()).to_int()); 
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

// CRARef Solver::propagate() {
//     CRARef conflict = CRAREF_UNDEF;

//     for (; _queue_head < _trail.size();) {
//         // 'p' is enqueued fact to propagate
//         Literal p = _trail[_queue_head++];


//     }
// }

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

    int sat, start = 1;

    for (; ;) {
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
            for (int i = 0; i < n_variables(); i++) {
                if (_assigns[i] == LIFTED_BOOLEAN_TRUE) {
                    printf("%d ", i + 1);
                } else {
                    printf("-%d ", i + 1);
                }
            }
            return LIFTED_BOOLEAN_TRUE;
        }

        if (start == 1) {
            start = 0;
            new_decision_level();
            Literal p = pick_branch_literal();
            unchecked_enqueue(p);
        } else {
            if (stat == LIFTED_BOOLEAN_UNDEF) {
                new_decision_level();
                Literal p = pick_branch_literal();
                unchecked_enqueue(p);
            } else {
                printf("Conflict\n");
                    for (int i = 0; i < n_variables(); i++) {
                        printf("%d ", _assigns[i].to_int());
                    }
                    printf("\n");
                printf("Back\n");
                for (; decision_level() != 0 && _trail[_trail.size() - 1].sign();) {
                    for (int i = 0; i < n_variables(); i++) {
                        printf("%d ", _assigns[i].to_int());
                    }
                    printf("\n");
                    cancel_until(decision_level() - 1);
                }
                printf("Back end\n");

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

// Public methods
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

// inline minor methods end

// major methods

Solver::Solver(void) :
    _myyura(true),
    _queue_head(0),
    _next_variable(0) {}

Solver::~Solver() {}

/**
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable 
 * will not be used as a decision variable (NOTE! This has effects on the 
 * meaning of a SATISFIABLE result).
 */
Variable Solver::new_variable(LiftedBoolean upol, bool dvar) {
    Variable v = _next_variable++;
    
    _assigns.insert(v, LIFTED_BOOLEAN_UNDEF);
    _variable_info.insert(v, _VariableInfo(CRAREF_UNDEF, 0));
    _polarity.insert(v, false);
    _trail.reserve(v + 1);

    return v;
}

bool Solver::solve_test(void) {
    LiftedBoolean status = search(100);
    if (status == LIFTED_BOOLEAN_TRUE) {
        printf("SAT\n");
    } else if (status == LIFTED_BOOLEAN_FALSE) {
        printf("UNSAT\n");
    }
}

void Solver::parse_dimacs(FILE* fp) {
    DimacsReader dr(fp);
    dr.parse_dimacs((*this));
}