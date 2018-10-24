/**
 * Lifted booleans
 */

#ifndef _MYYURASAT_LIFTED_BOOLEAN_H
#define _MYYURASAT_LIFTED_BOOLEAN_H

#include "literal.hpp"

namespace MyyuraSat {

class LiftedBoolean {
private:
    // 0: True, 1: False, 2: Undefined
    uint8_t _value;

public:
    LiftedBoolean(uint8_t v): _value(v) {}
    LiftedBoolean(): _value(0) {}
    LiftedBoolean(bool b): _value(!b) {}
    LiftedBoolean(int v): _value((uint8_t)v) {}

    inline bool operator==(const LiftedBoolean& lb) const {
        return ((lb._value & 2) & (_value & 2)) | (!(lb._value & 2) & (_value == lb._value));
    }
    inline bool operator!=(const LiftedBoolean& lb) const { return !(*this == lb); }
    inline LiftedBoolean operator^(bool b) const { 
        return LiftedBoolean((uint8_t)(_value ^ (uint8_t)b));
    }

    inline LiftedBoolean operator&&(const LiftedBoolean& lb) const {
        uint8_t sel = (_value << 1) | (lb._value << 3);
        uint8_t v = (0xF7F755F4 >> sel) & 3;
        return LiftedBoolean(v);
    }
    inline LiftedBoolean operator||(const LiftedBoolean& lb) const {
        uint8_t sel = (_value << 1) | (lb._value << 3);
        uint8_t v = (0xFCFCF400 >> sel) & 3;
        return LiftedBoolean(v);
    }

    inline int to_int(void) const { return _value; }
};

// some useful constants
const LiftedBoolean LIFTED_BOOLEAN_TRUE((uint8_t)0);
const LiftedBoolean LIFTED_BOOLEAN_FALSE((uint8_t)1);
const LiftedBoolean LIFTED_BOOLEAN_UNDEF((uint8_t)2);

}

#endif