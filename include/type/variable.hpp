/**
 * Variable
 * 
 * Basically our varalbles are just integers. Hence the variables also can be
 * used as array indices.
 */

#ifndef _MYYURASAT_VARIABLE_H
#define _MYYURASAT_VARIABLE_H

#include "../util/intmap.hpp"

namespace MyyuraSat {

using Variable = int32_t;
const Variable VARIABLE_UNDEF = -2;

template<typename T>
using VMap = IntMap<Variable, T>;

}

#endif