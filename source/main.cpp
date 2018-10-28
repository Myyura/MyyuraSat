#include "../include/core/solver.hpp"

#include <ctime>
#include <iostream>

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "r");
    MyyuraSat::Solver s;
    s.parse_dimacs(fp);
    int start_time = clock();
    s.solve_test();
    int end_time = clock();
    std::cout << (double)(end_time - start_time) / CLOCKS_PER_SEC << std::endl;
    return 0;
}