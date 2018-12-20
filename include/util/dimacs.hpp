/**
 * Parse dimacs files
 */

#ifndef _MYYURASAT_UNIT_PARSER_H
#define _MYYURASAT_UNIT_PARSER_H

#include "../core/solver.hpp"

#include <cstdio>
#include <stdexcept>
#include <new>

namespace {

class UnitParser {
private:
    FILE *_in;
    char* _current_line;
    int _pos;
    int _size;

public:
    explicit UnitParser(FILE *input, int line_size = 1024) : _in(input), _pos(0), _size(line_size) {
        _current_line = (char*)std::realloc(NULL, _size);
        if (_current_line == NULL) { throw std::bad_alloc(); }
    }

    ~UnitParser(void) { std::free(_current_line); }

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

    char current_char(void) {
        return _current_line[_pos];
    }
};

}

namespace MyyuraSat {

void parse_dimacs(FILE* in, Solver& s) {
    UnitParser p(in);
    Vector<Literal> lits;
    int vars = 0;
    int clauses = 0;
    int count = 0;

    for (; p.read_line();) {
        p.skip_whitespace();
        if (p.current_char() == '\0') {
            continue;
        }

        if (p.current_char() == 'p') {
            if (p.match("p cnf")) {
                vars = p.parse_integer();
                clauses = p.parse_integer();
            } else {
                fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", p.current_char());
                exit(3);
            }
        } else if (p.current_char() == 'c' || p.current_char() == 'p') {
            continue;
        } else {
            count++;

            // Read clause
            int parsed_lit, var;
            lits.clear();
            for (; ;) {
                parsed_lit = p.parse_integer();
                if (parsed_lit == 0) {
                    break;
                }
                var = std::abs(parsed_lit) - 1;

                for (; var >= s.n_variables();) {
                    // std::cout << var << std::endl;
                    s.new_variable();
                }

                lits.push((parsed_lit > 0) ? Literal(var) : ~Literal(var));
            }

            s.add_clause(lits);
        }
    }

    if (count != clauses) {
        fprintf(stderr, "PARSE ERROR! DIMACS header mismatch: wrong number ofclauses\n");
    }
}

}

#endif