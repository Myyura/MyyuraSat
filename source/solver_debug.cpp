/**
 * The SAT solver
 * The Debug Part
 */

#include "../include/core/solver.hpp"
#include "../include/util/algorithm.hpp"

using namespace MyyuraSat;

bool Solver::solve_test(void) {
    std::cout << "Start search! ==================" << std::endl;
    LiftedBoolean status = search(100);
    std::cout << "Finished search! ===============" << std::endl;
    if (status == LIFTED_BOOLEAN_TRUE) {
        for (int i = 0; i < n_variables(); i++) {
            if (_assigns[i] == LIFTED_BOOLEAN_TRUE) {
                printf("%d ", i + 1);
            } else {
                printf("-%d ", i + 1);
            }
        }
        printf("SAT\n");
    } else if (status == LIFTED_BOOLEAN_FALSE) {
        printf("UNSAT\n");
    }
}

void Solver::clause_test(void) {
    // subsume
    // Vector<Literal> A = {Literal(1, 0), Literal(2, 0), Literal(3, 0)};
    // Vector<Literal> B = {Literal(1, 1), Literal(2, 0)};

    Vector<Literal> A = {Literal(1, 0), Literal(2, 0), Literal(3, 0)};
    Vector<Literal> B = {Literal(1, 0), Literal(2, 0)};

    _ca.extra_clause_field(true);

    CRARef cr1 = _ca.alloc(A, false);
    CRARef cr2 = _ca.alloc(B, false);
    Clause& C1 = _ca[cr1];
    Clause& C2 = _ca[cr2];
    std::cout << C1.subsumes(C2).to_int() << std::endl;
    std::cout << C2.subsumes(C1).to_int() << std::endl;
    std::cout << LITERAL_UNDEF.to_int() << std::endl;
} 

void Solver::garbage_collection_test(void) {
    Vector<Literal> A = {Literal(1, 0), Literal(2, 0), Literal(3, 0)};
    Vector<Literal> B = {Literal(1, 0), Literal(2, 0)};
    Vector<Literal> C = {Literal(1, 0), Literal(2, 0), Literal(4, 0)};

    for (int i = 0; i <= 4; i++) {
        new_variable();
    }
    add_clause(A);
    add_clause(B);
    add_clause(C);

    print_clauses();
    remove_clause(_clauses[1]);
    print_clauses();
    garbage_collect();
    print_clauses();
}

void Solver::subsumption_test(void) {
    Vector<Literal> A = {Literal(1, 0), Literal(2, 0), Literal(3, 0)};
    Vector<Literal> B = {Literal(1, 0), Literal(2, 0)};
    Vector<Literal> C = {Literal(1, 0), Literal(2, 0), Literal(4, 0)};

    Vector<Literal> E = {Literal(2, 0), Literal(4, 0)};
    Vector<Literal> F = {Literal(4, 0), Literal(2, 0), Literal(5, 0)};
    Vector<Literal> G = {Literal(2, 0), Literal(4, 0)};
    Vector<Literal> D = {Literal(3, 0), Literal(2, 0)};

    for (int i = 0; i <= 5; i++) {
        new_variable();
    }

    _ca.extra_clause_field(true);

    print_clauses();

    add_clause(A);
    add_clause(B);
    add_clause(C);
    add_clause(D);
    add_clause(E);
    add_clause(F);
    add_clause(G);

    check_garbage();

    print_clauses();
    // reduction_subsumption(_clauses[1]);
    // print_clauses();


}