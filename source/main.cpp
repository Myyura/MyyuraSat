#include "../include/core/solver.hpp"
#include "../include/util/dimacs.hpp"

#include "./solver_basic.cpp"
#include "./solver_search.cpp"
#include "./solver_simplify.cpp"

#include "./solver_debug.cpp"

#include <ctime>
#include <iostream>

int main(int argc, char **argv) {
    // FILE *fp = fopen(argv[1], "r");
    MyyuraSat::Solver s;
    // s.add_empty_clause();
    // parse_dimacs(fp, s);
    // s.print_clauses();
    // int start_time = clock();
    // s.solve_test();
    // int end_time = clock();
    // std::cout << (double)(end_time - start_time) / CLOCKS_PER_SEC << std::endl;

    // s.clause_test();

    // s.garbage_collection_test();

    s.subsumption_test();
    return 0;
}