/*
 * Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com
*/

#ifndef CUBEMAP_UTIL_H
#define CUBEMAP_UTIL_H

#include <cmath>

#include "IlmBase/Imath/ImathVec.h"

// considers cubemaps to be in the following order:
// [right][left][top][bottom][back][front]
// This is how renderers like V-Ray or Keyshot output cubemaps.

// for coordinates, a right handed coordinate syste is considered with:
// Right:   +X
// Top:     +Y
// Forward: -Z

// uv coordinates are considered to start in the bottom left corner.

#define MIRROR_FACES 1
// if this is defined, each face of the cubemap is teated as horizontally mirrored (except for top and bottom, which are vertically mirrored),
// which again is how V-Ray and Keyshot output their cubemaps.

Imath::V2f sampleCube(
    const Imath::V3f& v,
    int* faceIndex)
{
    Imath::V3f vAbs = Imath::V3f(std::abs(v.x), std::abs(v.y), std::abs(v.z));
    float ma;
    Imath::V2f uv;
    if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
    { // +Z -Z
        *faceIndex = v.z < 0 ? 5 : 4;
        ma = 0.5f / vAbs.z;
        uv = Imath::V2f(v.z < 0 ? v.x : -v.x, v.y);
#ifdef MIRROR_FACES
        uv.x = -uv.x;
#endif
    }
    else if (vAbs.y >= vAbs.x)
    {
        *faceIndex = v.y < 0 ? 3 : 2;
        ma = 0.5f / vAbs.y;
        uv = Imath::V2f(v.x, v.y < 0 ? -v.z : v.z);
#ifdef MIRROR_FACES
        uv.y = -uv.y;
#endif
    }
    else
    {
        *faceIndex = v.x < 0 ? 1 : 0;
        ma = 0.5f / vAbs.x;
        uv = Imath::V2f(v.x < 0 ? -v.z : v.z, v.y);
#ifdef MIRROR_FACES
        uv.x = -uv.x;
#endif
    }
    return (uv * ma) + Imath::V2f(0.5f,0.5f);
}

/** Assumes that v is a unit vector. The result is a cubemap vector on the [0, 1] square. */
Imath::V2f cubeEncode(const Imath::V3f& v, int* face_out) {
    Imath::V2f uv = sampleCube(v, face_out);
    uv.x = std::clamp(uv.x, 0.0f, 1.0f);
    uv.y = std::clamp(uv.y, 0.0f, 1.0f);
    return uv;
}

/** Returns a unit vector. Argument o is an cubemap vector packed via cubeEncode,
    on the [0, +1] square*/
Imath::V3f cubeDecode(const Imath::V2f& o) {
    float tmp;
    float u = std::modf(o.x, &tmp);
    if (u < 0)
        u = 1.0f + u;
    float v = std::modf(o.y, &tmp);
    if (v < 0)
        v = 1.0f + u;

    Imath::V3f res;

    v = v * 2.0f - 1.0f;
    u *= 6.0f;
    if (u < 1.0f) { // Right, +X
        u = (u-0.5) * 2.0f;
#ifdef MIRROR_FACES
        u = -u;
#endif
       res.x = 1.0f;
       res.y = v;
       res.z = u;
    }
    else if (u < 2.0f) { // Left, -X
        u = (u-1.5f) * 2.0f;
#ifdef MIRROR_FACES
        u = -u;
#endif
        res.x = -1.0f;
        res.y = v;
        res.z = -u;
    }
    else if (u < 3.0f) { // Top, +Y
        u = (u-2.5f) * 2.0f;
#ifdef MIRROR_FACES
        v = -v;
#endif
        res.x = u;
        res.y = 1.0f;
        res.z = v;
    }
    else if (u < 4.0f) { // Bottom, -Y
        u = (u-3.5f) * 2.0f;
#ifdef MIRROR_FACES
        v = -v;
#endif
        res.x = u;
        res.y = -1.0f;
        res.z = -v;
    }
    else if (u < 5.0f) { // Back, +Z
        u = (u-4.5f) * 2.0f;
#ifdef MIRROR_FACES
        u = -u;
#endif
        res.x = -u;
        res.y = v;
        res.z = 1.0f;
    }
    else { // Front, -Z
        u = (u-5.5f) * 2.0f;
#ifdef MIRROR_FACES
        u = -u;
#endif
        res.x = u;
        res.y = v;
        res.z = -1.0f;
    }
    return res.normalized();
}

#endif  // CUBEMAP_UTIL_H