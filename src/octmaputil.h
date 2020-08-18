/*
 * Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com
*/

#ifndef OCTMAP_UTIL_H
#define OCTMAP_UTIL_H

#include <cmath>

#include "IlmBase/Imath/ImathVec.h"

float signNotZero(float k) {
    return (k >= 0.0f) ? 1.0f : -1.0f;
}

Imath::V2f signNotZero(const Imath::V2f& v) {
    return Imath::V2f(signNotZero(v.x), signNotZero(v.y));
}


// X+ maps to result.x+
// Y+ maps to result.y+
// Z+ maps to the center
// Z- maps to the corners

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
Imath::V2f octEncode(const Imath::V3f& v) {
    float l1norm = std::abs(v.x) + abs(v.y) + abs(v.z);
    float invL1norm = 1.0f / l1norm;
    Imath::V2f result(v.x * invL1norm, v.y * invL1norm);
    if (v.z < 0.0f) {
        result = (Imath::V2f(1.0f,1.0f) - Imath::V2f(std::abs(result.y), std::abs(result.x))) *
            signNotZero(Imath::V2f(result.x, result.y));
    }
    return result;
}

/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
Imath::V3f octDecode(const Imath::V2f& o) {
    Imath::V3f v(o.x, o.y, 1.0f - std::abs(o.x) - std::abs(o.y));
    if (v.z < 0.0f) {
        v.x = (1.0f - std::abs(o.y)) * signNotZero(o.x);
        v.y = (1.0f - std::abs(o.x)) * signNotZero(o.y);
    }
    v.normalize();
    return v;
}

#endif  // OCTMAP_UTIL_H