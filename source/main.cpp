#include "../include/core/solver.hpp"

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "r");
    MyyuraSat::Solver s;
    s.parse_dimacs(fp);
    s.solve_test();
    return 0;
}