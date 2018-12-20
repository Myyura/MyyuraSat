/**
 * Clauses
 * 
 * A simple class for representing clause
 */

#ifndef _MYYURASAT_CLAUSE_H
#define _MYYURASAT_CLAUSE_H

#include "literal.hpp"
#include "../util/alloc.hpp"
#include "../util/vector.hpp"
#include "../util/algorithm.hpp"

#include <unordered_map>
#include <unordered_set>

#include <iostream>

namespace MyyuraSat {

using CRARef = RegionAllocator<uint32_t>::RARef;

using CSet = std::unordered_set<CRARef>;

/**
 * Clause -- a simple class for representing a clause.
 */
class Clause {
private:
    /**
     * mark : clause is removed (1)
     */
    struct {
        unsigned mark      : 2;
        unsigned learnt    : 1;
        unsigned has_extra : 1;
        unsigned reloced   : 1;
        unsigned size      : 27; 
    } _header;

    union {
        Literal lit;
        float act;
        uint32_t abst;
        CRARef rel;
    } _data[0];

    friend class ClauseAllocator;

    Clause(const Vector<Literal>& ps, bool use_extra, bool learnt) {
        _header.mark = 0;
        _header.learnt = learnt;
        _header.has_extra = use_extra;
        _header.reloced = 0;
        _header.size = ps.size();

        for (int i = 0; i < ps.size(); i++) {
            _data[i].lit = ps[i];
        }

        if (_header.has_extra) {
            if (_header.learnt) {
                _data[_header.size].act = 0;
            } else {
                calc_abstraction();
            }
        }
    }

    Clause(const Clause& from, bool use_extra) {
        _header = from._header;
        _header.has_extra = use_extra;
        // NOTE: the copied clause may lose the extra field.

        for (int i = 0; i < from.size(); i++) {
            _data[i].lit = from[i];
        }

        if (_header.has_extra) {
            if (_header.learnt) {
                _data[_header.size].act = from._data[_header.size].act;
            } else {
                _data[_header.size].abst = from._data[_header.size].abst;
            }
        }
    }

public:
    // A 32-bit signature, [Bie]
    void calc_abstraction(void) {
        if (!_header.has_extra) {
            throw std::logic_error("Clause::calc_abstraction : has_extra is false");
        }

        uint32_t abstraction = 0;
        for (int i = 0; i < size(); i++) {
            abstraction |= _data[i].lit.abstraction();
        }
        _data[_header.size].abst = abstraction;
    }

    int size(void) const { return _header.size; }

    void shrink (int i) {
        if (i > size()) {
            throw std::out_of_range("Clause::shrink : out of size");
        }

        if (_header.has_extra) {
            _data[_header.size - i] = _data[_header.size];
        }

        _header.size -= i;
    }

    bool has(const Literal& p) const {
        for (int i = 0; i < size(); i++) {
            if ((*this)[i] == p) { return true; }
        }
        return false;
    }

    void pop(void) { shrink(1); }

    bool learnt(void) const { return _header.learnt; }

    bool has_extra(void) const { return _header.has_extra; }

    uint32_t mark(void) const { return _header.mark; }

    void mark(uint32_t m) { _header.mark = m; }

    const Literal& last(void) const { return _data[_header.size - 1].lit; }

    bool reloced(void) const { return _header.reloced; }

    CRARef relocation(void) const { return _data[0].rel; }

    void relocate(CRARef c) {
        _header.reloced = 1;
        _data[0].rel = c;
    }


    /**
     * NOTE: somewhat unsafe to change the clause in-place! Must manually call '
     * calcAbstraction' afterwards for subsumption operations to behave correctly.
     */

    Literal& operator[](int i) { return _data[i].lit; }
    Literal operator[](int i) const { return _data[i].lit; }
    operator const Literal* (void) const { return (Literal*)_data; }

    float& activity(void) {
        if (!_header.has_extra) {
            throw std::logic_error("Clause::activity : no extras");
        }

        return _data[_header.size].act;
    }

    uint32_t abstraction(void) const {
        if (!_header.has_extra) {
            throw std::logic_error("Clause::abstraction : no extras");
        }

        return _data[_header.size].abst;
    }

    Literal subsumes(const Clause& other) const;

    void strengthen(Literal p) {
        remove(*this, p);
        calc_abstraction();
    }
};

/**
 * subsumes : (other : const Clause&)  ->  Lit
 *
 * Description:
 *     Checks if clause subsumes 'other', and at the same time, if it can be 
 * used to simplify 'other' by subsumption resolution.
 *
 * Result:
 *     LITERAL_ERROR - No subsumption or simplification
 *     LITERAL_UNDEF - Clause subsumes 'other'
 *     Literal p     - The literal p can be deleted from 'other'
 * 
 * Reference:
 * [EB05] N.Een, A. Biere. "Effiective preprocessing in SAT through variable and
 * clause elimination". Proc. of SAT, 2005
 * 
 * [Bie04] A. Biere. Resolve and expand. In Prel. Proc. of SAT 2004
 */

inline Literal Clause::subsumes(const Clause& other) const {
    if ((_header.learnt || other._header.learnt) ||
        (!_header.has_extra || !other._header.has_extra)) {
        throw std::logic_error("Clause::subsumes(const Clause& other) - learnt clause error or no extra spaces");
    }

    if (other._header.size < _header.size ||
        (_data[_header.size].abst & ~other._data[other._header.size].abst) != 0) {
        return LITERAL_ERROR;
    }

    Literal result = LITERAL_UNDEF;

    for (int i = 0; i < _header.size; i++) {
        // search for _data[i].lit or ~_data[i].lit
        bool is_found = false;
        for (int j = 0; j < other._header.size; j++) {
            if (_data[i].lit == other._data[j].lit) {
                is_found = true;
                break;
            } else if (result == LITERAL_UNDEF && _data[i].lit == ~other._data[j].lit) {
                result = _data[i].lit;
                is_found = true;
                break;
            }
        }

        if (!is_found) { return LITERAL_ERROR; }
    }

    return result;
}


/**
 * ClauseAllocator -- a simple class for allocating memory for clauses.
 */
const CRARef CRAREF_UNDEF = RegionAllocator<uint32_t>::RAREF_UNDEF;

class ClauseAllocator {
private:
    RegionAllocator<uint32_t> _ra;
    bool _extra_clause_field;

    uint32_t clause_word32size(int size, bool has_extra) {
        return (sizeof(Clause) + (sizeof(Literal) * (size + (int)has_extra))) / sizeof(uint32_t);
    }

public:
    static const std::size_t UNIT_SIZE = RegionAllocator<uint32_t>::UNIT_SIZE;

    ClauseAllocator(uint32_t start_cap) : _ra(start_cap), _extra_clause_field(false) {}

    ClauseAllocator(void) : _extra_clause_field(false) {}

    void move_to(ClauseAllocator& to) {
        to._extra_clause_field = _extra_clause_field;
        _ra.move_to(to._ra);
    }

    CRARef alloc(const Vector<Literal>& ps, bool learnt = false) {
        if (sizeof(Literal) != sizeof(uint32_t) || sizeof(float) != sizeof(uint32_t)) {
            throw;
        }

        bool use_extra = learnt | _extra_clause_field;
        CRARef cid = _ra.alloc(clause_word32size(ps.size(), use_extra));
        new (lea(cid)) Clause(ps, use_extra, learnt);

        return cid;
    }

    CRARef alloc(const Clause& from) {
        bool use_extra = from.learnt() | _extra_clause_field;
        CRARef cid = _ra.alloc(clause_word32size(from.size(), use_extra));
        new (lea(cid)) Clause(from, use_extra);
        return cid;
    }

    uint32_t size(void) const { return _ra.size(); }
    uint32_t wasted(void) const { return _ra.wasted(); }

    Clause& operator[](CRARef r) { return (Clause&)_ra[r]; }
    const Clause& operator[](CRARef r) const { return (Clause&)_ra[r]; }

    Clause *lea(CRARef r) { return (Clause*)_ra.lea(r); }
    const Clause *lea(CRARef r) const { return (Clause*)_ra.lea(r); }

    void free(CRARef cid) {
        Clause& c = operator[](cid);
        _ra.free(clause_word32size(c.size(), c.has_extra()));
    }

    void reloc(CRARef& cr, ClauseAllocator& to) {
        Clause& c = operator[](cr);

        if (c.reloced()) { 
            cr = c.relocation();
            return;
        }

        cr = to.alloc(c);
        c.relocate(cr);
    }

    void extra_clause_field(bool use_extra) { _extra_clause_field = use_extra; }
};

/**
 * ClauseMap -- a class for mapping clauses to values.
 */
template<typename T>
class ClauseMap {
private:
    struct _CRefHash {
        std::size_t operator()(CRARef cr) const { return (std::size_t)cr; }
    };

    using _HashTable = std::unordered_map<CRARef, T, _CRefHash>;
    _HashTable _map;

public:
    void clear(void) { _map.clear(); }
    
    int size(void) const { return _map.size(); }

    // Modifiers
    void insert(CRARef cr, const T& t) { _map.insert(cr, t); }

    void remove(CRARef cr) { _map.erase(cr); }

    // Find
    bool has(CRARef cr, T& t) {
        if (_map.count(cr) == 0) {
            return false;
        }
        
        t = _map[cr];
        return true;
    }

    bool has(CRARef cr) {
        return _map.count(cr) > 0;
    }

    // Access
    const T& operator[](CRARef cr) const { return _map[cr]; }
    T& operator[](CRARef cr) { return _map[cr]; }

    // Bucket
    // int bucket_count(void) const { return _map.bucket_count(); }
};

}

#endif