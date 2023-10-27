/* gb_math.h - v0.07c - public domain C math library - no warranty implied; use at your own risk
   A C math library geared towards game development
   use '#define GB_MATH_IMPLEMENTATION' before including to create the implementation in _ONE_ file

Version History:
    0.07f - Fix constants
    0.07e - Fixed a warning
    0.07d - Fix mat4_inverse
    0.07c - Add gb_random01
    0.07b - Fix mat4_inverse
    0.07a - Fix Mat2
    0.07  - Better Mat4 procedures
    0.06h - Ignore silly warnings
    0.06g - Remove memzero
    0.06f - Remove warning on MSVC
    0.06e - Change brace style and fix some warnings
    0.06d - Bug fix
    0.06c - Remove extra needed define for C++ and inline all operators
    0.06b - Just formatting
    0.06a - Implement rough versions of mod, remainder, copy_sign
    0.06  - Windows GCC Support and C90-ish Support
    0.05  - Less/no dependencies or CRT
    0.04d - License Update
    0.04c - Use 64-bit murmur64 version on WIN64
    0.04b - Fix strict aliasing in gb_quake_rsqrt
    0.04a - Minor bug fixes
    0.04  - Namespace everything with gb
    0.03  - Complete Replacement
    0.01  - Initial Version

LICENSE
    This software is dual-licensed to the public domain and under the following
    license: you are granted a perpetual, irrevocable license to copy, modify,
    publish, and distribute this file as you see fit.
WARNING
    - This library is _slightly_ experimental and features may not work as expected.
    - This also means that many functions are not documented.

CONTENTS
    - Common Macros
    - Types
        - gbVec(2,3,4)
        - gbMat(2,3,4)
        - gbFloat(2,3,4)
        - gbQuat
        - gbRect(2,3)
        - gbAabb(2,3)
        - gbHalf (16-bit floating point) (storage only)
    - Operations
    - Functions
    - Type Functions
    - Random
    - Hash
*/

#ifndef GB_MATH_INCLUDE_GB_MATH_H
#define GB_MATH_INCLUDE_GB_MATH_H

#include <stddef.h>
#include <cassert>

#if !defined(GB_MATH_NO_MATH_H)
    #include <math.h>
#else
    #include <intrin.h>
#endif

#define GB_MATH_DEF [[nodiscard]]

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4201)
#endif

template<typename T>
union gbVec2 {
    struct { T x, y; };
    T e[2];
};

template<typename T>
union gbVec3 {
    struct { T x, y, z; };
    struct { T r, g, b; };

    gbVec2<T> xy;
    T e[3];
};

template<typename T>
union gbVec4 {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    struct { gbVec2<T> xy, zw; };
    gbVec3<T> xyz;
    gbVec3<T> rgb;
    T e[4];
};



template<typename T>
union gbMat2 {
    struct { gbVec2<T> x, y; };
    gbVec2<T> col[2];
    T e[4];
    T d[2][2];
};

template<typename T>
union gbMat3 {
    struct { gbVec3<T> x, y, z; };
    gbVec3<T> col[3];
    T e[9];
    T d[3][3];
};

template<typename T>
union gbMat4 {
    struct { gbVec4<T> x, y, z, w; };
    gbVec4<T> col[4];
    T e[16];
    T d[4][4];
};


template<typename T>
union gbQuat {
    struct { T x, y, z, w; };
    gbVec4<T> xyzw;
    gbVec3<T> xyz;
    T e[4];
};


#if defined(_MSC_VER)
#pragma warning(pop)
#endif

typedef float gbFloat2[2];
typedef float gbFloat3[3];
typedef float gbFloat4[4];

template<typename T> struct gbRect2 { gbVec2<T> pos, dim; };
template<typename T> struct gbRect3 { gbVec3<T> pos, dim; };

template<typename T> struct gbAabb2 { gbVec2<T> centre, half_size; };
template<typename T> struct gbAabb3 { gbVec3<T> centre, half_size; };

#if defined(_MSC_VER)
    typedef unsigned __int32 gb_math_u32;
    typedef unsigned __int64 gb_math_u64;
#else
    #if defined(GB_USE_STDINT)
        #include <stdint.h>
        typedef uint32_t gb_math_u32;
        typedef uint64_t gb_math_u64;
    #else
        typedef unsigned int       gb_math_u32;
        typedef unsigned long long gb_math_u64;
    #endif
#endif

typedef short gbHalf;


#ifndef GB_MATH_CONSTANTS
#define GB_MATH_CONSTANTS
    #define GB_MATH_EPSILON      1.19209290e-7f
    #define GB_MATH_ZERO         0.0f
    #define GB_MATH_ONE          1.0f
    #define GB_MATH_TWO_THIRDS   0.666666666666666666666666666666666666667f

    #define GB_MATH_TAU          6.28318530717958647692528676655900576f
    #define GB_MATH_PI           3.14159265358979323846264338327950288f
    #define GB_MATH_ONE_OVER_TAU 0.159154943091895335768883763372514362f
    #define GB_MATH_ONE_OVER_PI  0.318309886183790671537767526745028724f

    #define GB_MATH_TAU_OVER_2   3.14159265358979323846264338327950288f
    #define GB_MATH_TAU_OVER_4   1.570796326794896619231321691639751442f
    #define GB_MATH_TAU_OVER_8   0.785398163397448309615660845819875721f

    #define GB_MATH_E            2.7182818284590452353602874713526625f
    #define GB_MATH_SQRT_TWO     1.41421356237309504880168872420969808f
    #define GB_MATH_SQRT_THREE   1.73205080756887729352744634150587236f
    #define GB_MATH_SQRT_FIVE    2.23606797749978969640917366873127623f

    #define GB_MATH_LOG_TWO      0.693147180559945309417232121458176568f
    #define GB_MATH_LOG_TEN      2.30258509299404568401799145468436421f
#endif


#ifndef gb_clamp
#define gb_clamp(x, lower, upper) (gb_min(gb_max(x, (lower)), (upper)))
#endif
#ifndef gb_clamp01
#define gb_clamp01(x) gb_clamp(x, 0, 1)
#endif

#ifndef gb_square
#define gb_square(x) ((x)*(x))
#endif

#ifndef gb_cube
#define gb_cube(x) ((x)*(x)*(x))
#endif

#ifndef gb_abs
#define gb_abs(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef gb_sign
#define gb_sign(x) ((x) >= 0 ? 1 : -1)
#endif


GB_MATH_DEF float gb_to_radians(const float degrees);
GB_MATH_DEF float gb_to_degrees(const float radians);

/* NOTE(bill): Because to interpolate angles */
GB_MATH_DEF float gb_angle_diff(const float radians_a, const float radians_b);

#ifndef gb_min
#define gb_min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef gb_max
#define gb_max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef gb_min3
#define gb_min3(a, b, c) gb_min(gb_min(a, b), c)
#endif

#ifndef gb_max3
#define gb_max3(a, b, c) gb_max(gb_max(a, b), c)
#endif


GB_MATH_DEF float gb_copy_sign  (const float x, const float y);
GB_MATH_DEF float gb_remainder  (const float x, const float y);
GB_MATH_DEF float gb_mod        (const float x, float y);
GB_MATH_DEF float gb_sqrt       (const float a);
GB_MATH_DEF float gb_rsqrt      (const float a);
GB_MATH_DEF float gb_quake_rsqrt(const float a); /* NOTE(bill): It's probably better to use 1.0f/gb_sqrt(a)
                                            * And for simd, there is usually isqrt functions too!
                                            */
GB_MATH_DEF float gb_sin    (const float radians);
GB_MATH_DEF float gb_cos    (const float radians);
GB_MATH_DEF float gb_tan    (const float radians);
GB_MATH_DEF float gb_arcsin (const float a);
GB_MATH_DEF float gb_arccos (const float a);
GB_MATH_DEF float gb_arctan (const float a);
GB_MATH_DEF float gb_arctan2(const float y, const float x);

GB_MATH_DEF float gb_exp      (const float x);
GB_MATH_DEF float gb_exp2     (const float x);
GB_MATH_DEF float gb_log      (const float x);
GB_MATH_DEF float gb_log2     (const float x);
GB_MATH_DEF float gb_fast_exp (const float x);  /* NOTE(bill): Only valid from -1 <= x <= +1 */
GB_MATH_DEF float gb_fast_exp2(const float x); /* NOTE(bill): Only valid from -1 <= x <= +1 */
GB_MATH_DEF float gb_pow      (const float x, const float y); /* x^y */

GB_MATH_DEF float gb_round(const float x);
GB_MATH_DEF float gb_floor(const float x);
GB_MATH_DEF float gb_ceil (const float x);

GB_MATH_DEF float  gb_half_to_float(const gbHalf value);
GB_MATH_DEF gbHalf gb_float_to_half(const float value);

template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_zero(void)   { gbVec2<T> v = { };             return v; }
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2(T x, T y)    { gbVec2<T> v; v.x = x;    v.y = y;    return v; }
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2v(T x[2])     { gbVec2<T> v; v.x = x[0]; v.y = x[1]; return v; }

template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_zero(void)       { gbVec3<T> v = {0, 0, 0};                         return v; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3(T x, T y, T z)   { gbVec3<T> v; v.x = x; v.y = y; v.z = z;          return v; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3v(T x[3])         { gbVec3<T> v; v.x = x[0]; v.y = x[1]; v.z = x[2]; return v; }
                     
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_zero(void)           { gbVec4<T> v = {0, 0, 0, 0};                                  return v; }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4(T x, T y, T z, T w)  { gbVec4<T> v; v.x = x; v.y = y; v.z = z; v.w = w;             return v; }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4v(T x[4])             { gbVec4<T> v; v.x = x[0]; v.y = x[1]; v.z = x[2]; v.w = x[3]; return v; }

#define GB_VEC2_2OP(a,c,post)  \
    a.x =         c.x post;    \
    a.y =         c.y post;

#define GB_VEC2_3OP(a,b,op,c,post) \
                                    \
    a.x = b.x op c.x post;        \
    a.y = b.y op c.y post;

#define GB_VEC3_2OP(a,c,post) \
    a.x =        c.x post;   \
    a.y =        c.y post;   \
    a.z =        c.z post;

#define GB_VEC3_3OP(a,b,op,c,post) \
    a.x = b.x op c.x post;        \
    a.y = b.y op c.y post;        \
    a.z = b.z op c.z post;

#define GB_VEC4_2OP(a,c,post) \
    a.x =        c.x post;   \
    a.y =        c.y post;   \
    a.z =        c.z post;   \
    a.w =        c.w post;

#define GB_VEC4_3OP(a,b,op,c,post) \
    a.x = b.x op c.x post;        \
    a.y = b.y op c.y post;        \
    a.z = b.z op c.z post;        \
    a.w = b.w op c.w post;

template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_add(const gbVec2<T>& v0, const gbVec2<T>& v1){ gbVec2<T> r; GB_VEC2_3OP(r,v0,+,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_sub(const gbVec2<T>& v0, const gbVec2<T>& v1){ gbVec2<T> r; GB_VEC2_3OP(r,v0,-,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_mul(const gbVec2<T>& v,  const T s)          { gbVec2<T> r; GB_VEC2_2OP(r,v,* s);      return r; }
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_div(const gbVec2<T>& v,  const T s)          { gbVec2<T> r; GB_VEC2_2OP(r,v,/ s);      return r; }

template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_add(const gbVec3<T>& v0, const gbVec3<T>& v1){ gbVec3<T> r; GB_VEC3_3OP(r,v0,+,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_sub(const gbVec3<T>& v0, const gbVec3<T>& v1){ gbVec3<T> r; GB_VEC3_3OP(r,v0,-,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_mul(const gbVec3<T>& v,  const T s)          { gbVec3<T> r; GB_VEC3_2OP(r,v,* s);      return r; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_div(const gbVec3<T>& v,  const T s)          { gbVec3<T> r; GB_VEC3_2OP(r,v,/ s);      return r; }

template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_add(const gbVec4<T>& v0, const gbVec4<T>& v1){ gbVec4<T> r; GB_VEC4_3OP(r,v0,+,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_sub(const gbVec4<T>& v0, const gbVec4<T>& v1){ gbVec4<T> r; GB_VEC4_3OP(r,v0,-,v1,+0); return r; }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_mul(const gbVec4<T>& v,  const T s)          { gbVec4<T> r; GB_VEC4_2OP(r,v,* s);      return r; }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_div(const gbVec4<T>& v,  const T s)          { gbVec4<T> r; GB_VEC4_2OP(r,v,/ s);      return r; }


template<typename T> GB_MATH_DEF void gb_vec2_addeq(gbVec2<T>& d, const gbVec2<T>& v)   { GB_VEC2_3OP(d,d,+,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec2_subeq(gbVec2<T>& d, const gbVec2<T>& v)   { GB_VEC2_3OP(d,d,-,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec2_muleq(gbVec2<T>& d, const T s)            { GB_VEC2_2OP(d,d,* s);    }
template<typename T> GB_MATH_DEF void gb_vec2_diveq(gbVec2<T>& d, const T s)            { GB_VEC2_2OP(d,d,/ s);    }

template<typename T> GB_MATH_DEF void gb_vec3_addeq(gbVec3<T>& d, const gbVec3<T>& v)   { GB_VEC3_3OP(d,d,+,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec3_subeq(gbVec3<T>& d, const gbVec3<T>& v)   { GB_VEC3_3OP(d,d,-,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec3_muleq(gbVec3<T>& d, const T s)            { GB_VEC3_2OP(d,d,* s);    }
template<typename T> GB_MATH_DEF void gb_vec3_diveq(gbVec3<T>& d, const T s)            { GB_VEC3_2OP(d,d,/ s);    }

template<typename T> GB_MATH_DEF void gb_vec4_addeq(gbVec4<T>& d, const gbVec4<T>& v)   { GB_VEC4_3OP(d,d,+,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec4_subeq(gbVec4<T>& d, const gbVec4<T>& v)   { GB_VEC4_3OP(d,d,-,v,+0); }
template<typename T> GB_MATH_DEF void gb_vec4_muleq(gbVec4<T>& d, const T s)            { GB_VEC4_2OP(d,d,* s);    }
template<typename T> GB_MATH_DEF void gb_vec4_diveq(gbVec4<T>& d, const T s)            { GB_VEC4_2OP(d,d,/ s);    }

#undef GB_VEC2_2OP
#undef GB_VEC2_3OP
#undef GB_VEC3_3OP
#undef GB_VEC3_2OP
#undef GB_VEC4_2OP
#undef GB_VEC4_3OP

template<typename T> GB_MATH_DEF T gb_vec2_dot(const gbVec2<T>& v0, const gbVec2<T>& v1) { return v0.x*v1.x + v0.y*v1.y; }
template<typename T> GB_MATH_DEF T gb_vec3_dot(const gbVec3<T>& v0, const gbVec3<T>& v1) { return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z; }
template<typename T> GB_MATH_DEF T gb_vec4_dot(const gbVec4<T>& v0, const gbVec4<T>& v1) { return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z + v0.w*v1.w; }

template<typename T> GB_MATH_DEF T  gb_vec2_cross(const gbVec2<T>& v0, const gbVec2<T>& v1)  { return v0.x*v1.y - v1.x*v0.y; }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_cross(const gbVec3<T>& v0, const gbVec3<T>& v1) { return { v0.y * v1.z - v0.z * v1.y, 
                                                                    v0.z * v1.x - v0.x * v1.z, 
                                                                    v0.x * v1.y - v0.y * v1.x }; }

template<typename T> GB_MATH_DEF T gb_vec2_mag2(const gbVec2<T>& v) { return gb_vec2_dot(v, v); }
template<typename T> GB_MATH_DEF T gb_vec3_mag2(const gbVec3<T>& v) { return gb_vec3_dot(v, v); }
template<typename T> GB_MATH_DEF T gb_vec4_mag2(const gbVec4<T>& v) { return gb_vec4_dot(v, v); }

/* TODO(bill): Create custom sqrt function */
template<typename T> GB_MATH_DEF T gb_vec2_mag(const gbVec2<T>& v) { return gb_sqrt(gb_vec2_dot(v, v)); }
template<typename T> GB_MATH_DEF T gb_vec3_mag(const gbVec3<T>& v) { return gb_sqrt(gb_vec3_dot(v, v)); }
template<typename T> GB_MATH_DEF T gb_vec4_mag(const gbVec4<T>& v) { return gb_sqrt(gb_vec4_dot(v, v)); }

template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_norm(const gbVec2<T>& v) {
    T inv_mag = gb_rsqrt(gb_vec2_dot(v, v));
    return gb_vec2_mul(v, inv_mag);
}
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_norm(const gbVec3<T>& v) {
    T mag = gb_vec3_mag(v);
    return gb_vec3_div(v, mag);
}
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_norm(const gbVec4<T>& v) {
    T mag = gb_vec4_mag(v);
    return gb_vec4_div(v, mag);
}

template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_norm0(const gbVec2<T>& v) {
    T mag = gb_vec2_mag(v);
    if (mag > 0)
        return gb_vec2_div(v, mag);
    return gb_vec2_zero<T>();
}
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_norm0(const gbVec3<T>& v) {
    T mag = gb_vec3_mag(v);
    if (mag > 0)
        return gb_vec3_div(v, mag);
    return gb_vec3_zero<T>();
}
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_norm0(const gbVec4<T>& v) {
    T mag = gb_vec4_mag(v);
    if (mag > 0)
        return gb_vec4_div(v, mag);
    else
        return gb_vec4_zero<T>();
}


template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_reflect(const gbVec2<T>& i, gbVec2<T> n) {
    gb_vec2_muleq(n, 2.0f*gb_vec2_dot(n, i));
    return gb_vec2_sub(i, n);
}

template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_reflect(const gbVec3<T>& i, gbVec3<T> n) {
    gb_vec3_muleq(n, 2.0f*gb_vec3_dot(n, i));
    return gb_vec3_sub(i, n);
}

template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_refract(const gbVec2<T>& i, const gbVec2<T>& n, const T eta) {
    T dv = gb_vec2_dot(n, i);
    T k = 1.0f - eta*eta * (1.0f - dv*dv);
    gbVec2<T> a = gb_vec2_mul(i, eta);
    gbVec2<T> b = gb_vec2_mul(n, eta*dv*gb_sqrt(k));
    gbVec2<T> r = gb_vec2_sub(a, b);
    gb_vec2_muleq(r, (T)(k >= 0.0f));
    return r;
}

template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_refract(const gbVec3<T>& i, const gbVec3<T>& n, const T eta) {
    T dv = gb_vec3_dot(n, i);
    T k = 1.0f - eta*eta * (1.0f - dv*dv);
    gbVec3<T> a = gb_vec3_mul(i, eta);
    gbVec3<T> b = gb_vec3_mul(n, eta*dv*gb_sqrt(k));
    gbVec3<T> r = gb_vec3_sub(a, b);
    gb_vec3_muleq(r, (T)(k >= 0.0f));
    return r;
}



template<typename T> GB_MATH_DEF T gb_vec2_aspect_ratio(const gbVec2<T>& v) { return (v.y < 0.0001f) ? 0.0f : v.x/v.y; }


/*******
// MAT2
********/
template<typename T> GB_MATH_DEF void gb_mat2_identity(gbMat2<T>& m) {
    m.d[0][0] = 1; m.d[0][1] = 0;
    m.d[1][0] = 0; m.d[1][1] = 1;
}
template<typename T> GB_MATH_DEF void gb_mat2_transpose(gbMat2<T>& mat) {
    for (int j = 0; j < 2; j++) {
        for (int i = j + 1; i < 2; i++) {
            T t		= mat.d[i][j];
            mat.d[i][j] = mat.d[j][i];
            mat.d[j][i] = t;
        }
    }
}

template<typename T> GB_MATH_DEF gbMat2<T> gb_mat2_mul(const gbMat2<T>& mat1, const gbMat2<T>& mat2) {
    gbMat2<T> r;
    r.d[0][0] = (mat1.d[0][0] * mat2.d[0][0]) + (mat1.d[1][0] * mat2.d[0][1]);
    r.d[0][1] = (mat1.d[0][1] * mat2.d[0][0]) + (mat1.d[1][1] * mat2.d[0][1]);
    r.d[1][0] = (mat1.d[0][0] * mat2.d[1][0]) + (mat1.d[1][0] * mat2.d[1][1]);
    r.d[1][1] = (mat1.d[0][1] * mat2.d[1][0]) + (mat1.d[1][1] * mat2.d[1][1]);
    return r;
}

template<typename T> GB_MATH_DEF gbVec2<T> gb_mat2_mul_vec2(const gbMat2<T>& m, const gbVec2<T>& v)
{
    return {m.d[0][0] * v.x + m.d[0][1] * v.y,
            m.d[1][0] * v.x + m.d[1][1] * v.y };
}

template<typename T> GB_MATH_DEF gbMat2<T> gb_mat2_inverse(const gbMat2<T>& in) {
    T ood = 1.0f / gb_mat2_determinate(in);
    gbMat2<T> r;
    r.d[0][0] = +in.d[1][1] * ood;
    r.d[0][1] = -in.d[0][1] * ood;
    r.d[1][0] = -in.d[1][0] * ood;
    r.d[1][1] = +in.d[0][0] * ood;
    return r;
}

template<typename T> GB_MATH_DEF T gb_mat2_determinate(const gbMat2<T>& m) {
    return m.d[0][0]*m.d[1][1] - m.d[1][0]*m.d[0][1];
}


/*******
// MAT3
********/

template<typename T> GB_MATH_DEF void gb_mat3_identity(gbMat3<T>& m)  { 
    m.d[0][0] = 1; m.d[0][1] = 0; m.d[0][2] = 0;
    m.d[1][0] = 0; m.d[1][1] = 1; m.d[1][2] = 0;
    m.d[2][0] = 0; m.d[2][1] = 0; m.d[2][2] = 1;
}

template<typename T> GB_MATH_DEF void gb_mat3_transpose(gbMat3<T>& m) {
    for (int j = 0; j < 3; j++) {
        for (int i = j + 1; i < 3; i++) {
            T t = m.d[i][j];
            m.d[i][j] = m.d[j][i];
            m.d[j][i] = t;
        }
    }
}

template<typename T> GB_MATH_DEF gbMat3<T> gb_mat3_mul(const gbMat3<T>& m1, const gbMat3<T>& m2) {
    gbMat3<T> r;
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            r.d[j][i] = m1.d[0][i]*m2.d[j][0]
                      + m1.d[1][i]*m2.d[j][1]
                      + m1.d[2][i]*m2.d[j][2];
        }
    }
    return r;
}

template<typename T> GB_MATH_DEF gbVec3<T> gb_mat3_mul_vec3(const gbMat3<T>& m, const gbVec3<T>& v) { 
    gbVec3<T> r;
    r.x = m.d[0][0]*v.x + m.d[0][1]*v.y + m.d[0][2]*v.z;
    r.y = m.d[1][0]*v.x + m.d[1][1]*v.y + m.d[1][2]*v.z;
    r.z = m.d[2][0]*v.x + m.d[2][1]*v.y + m.d[2][2]*v.z;
    return r;
}

template<typename T> GB_MATH_DEF gbMat3<T> gb_mat3_inverse(const gbMat3<T>& in) {
    T ood = 1.0f / gb_mat3_determinate(in);
    
    gbMat3<T> r;
    r.d[0][0] = +(in.d[1][1] * in.d[2][2] - in.d[2][1] * in.d[1][2]) * ood;
    r.d[0][1] = -(in.d[1][0] * in.d[2][2] - in.d[2][0] * in.d[1][2]) * ood;
    r.d[0][2] = +(in.d[1][0] * in.d[2][1] - in.d[2][0] * in.d[1][1]) * ood;
    r.d[1][0] = -(in.d[0][1] * in.d[2][2] - in.d[2][1] * in.d[0][2]) * ood;
    r.d[1][1] = +(in.d[0][0] * in.d[2][2] - in.d[2][0] * in.d[0][2]) * ood;
    r.d[1][2] = -(in.d[0][0] * in.d[2][1] - in.d[2][0] * in.d[0][1]) * ood;
    r.d[2][0] = +(in.d[0][1] * in.d[1][2] - in.d[1][1] * in.d[0][2]) * ood;
    r.d[2][1] = -(in.d[0][0] * in.d[1][2] - in.d[1][0] * in.d[0][2]) * ood;
    r.d[2][2] = +(in.d[0][0] * in.d[1][1] - in.d[1][0] * in.d[0][1]) * ood;
    return r;
}

template<typename T> GB_MATH_DEF T gb_mat3_determinate(const gbMat3<T>& m) {
    T r = +m.d[0][0] * (m.d[1][1] * m.d[2][2] - m.d[1][2] * m.d[2][1])
              -m.d[0][1] * (m.d[1][0] * m.d[2][2] - m.d[1][2] * m.d[2][0])
              +m.d[0][2] * (m.d[1][0] * m.d[2][1] - m.d[1][1] * m.d[2][0]);
    return r;
}


/*******
// MAT4
********/


template<typename T> GB_MATH_DEF void gb_mat4_identity(gbMat4<T>& m)  {
    m.d[0][0] = 1; m.d[0][1] = 0; m.d[0][2] = 0; m.d[0][3] = 0;
    m.d[1][0] = 0; m.d[1][1] = 1; m.d[1][2] = 0; m.d[1][3] = 0;
    m.d[2][0] = 0; m.d[2][1] = 0; m.d[2][2] = 1; m.d[2][3] = 0;
    m.d[3][0] = 0; m.d[3][1] = 0; m.d[3][2] = 0; m.d[3][3] = 1;
}
template<typename T> GB_MATH_DEF void gb_mat4_transpose(gbMat4<T>& m) { 
    T tmp;
    tmp = m.d[1][0]; m.d[1][0] = m.d[0][1]; m.d[0][1] = tmp;
    tmp = m.d[2][0]; m.d[2][0] = m.d[0][2]; m.d[0][2] = tmp;
    tmp = m.d[3][0]; m.d[3][0] = m.d[0][3]; m.d[0][3] = tmp;
    tmp = m.d[2][1]; m.d[2][1] = m.d[1][2]; m.d[1][2] = tmp;
    tmp = m.d[3][1]; m.d[3][1] = m.d[1][3]; m.d[1][3] = tmp;
    tmp = m.d[3][2]; m.d[3][2] = m.d[2][3]; m.d[2][3] = tmp;
}
template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_mul(const gbMat4<T>& m1, const gbMat4<T>& m2) { 
    gbMat4<T> r;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            r.d[j][i] = m1.d[0][i]*m2.d[j][0]
                      + m1.d[1][i]*m2.d[j][1]
                      + m1.d[2][i]*m2.d[j][2]
                      + m1.d[3][i]*m2.d[j][3];
        }
    }
    return r;
}


template<typename T> GB_MATH_DEF gbVec4<T> gb_mat4_mul_vec4(const gbMat4<T>& m, const gbVec4<T>& v) { 
    gbVec4<T> r;
    r.x = m.d[0][0]*v.x + m.d[1][0]*v.y + m.d[2][0]*v.z + m.d[3][0]*v.w;
    r.y = m.d[0][1]*v.x + m.d[1][1]*v.y + m.d[2][1]*v.z + m.d[3][1]*v.w;
    r.z = m.d[0][2]*v.x + m.d[1][2]*v.y + m.d[2][2]*v.z + m.d[3][2]*v.w;
    r.w = m.d[0][3]*v.x + m.d[1][3]*v.y + m.d[2][3]*v.z + m.d[3][3]*v.w;
    return r;
}




template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_inverse(const gbMat4<T>& m) {
    T sf00 = m.d[2][2] * m.d[3][3] - m.d[3][2] * m.d[2][3];
    T sf01 = m.d[2][1] * m.d[3][3] - m.d[3][1] * m.d[2][3];
    T sf02 = m.d[2][1] * m.d[3][2] - m.d[3][1] * m.d[2][2];
    T sf03 = m.d[2][0] * m.d[3][3] - m.d[3][0] * m.d[2][3];
    T sf04 = m.d[2][0] * m.d[3][2] - m.d[3][0] * m.d[2][2];
    T sf05 = m.d[2][0] * m.d[3][1] - m.d[3][0] * m.d[2][1];
    T sf06 = m.d[1][2] * m.d[3][3] - m.d[3][2] * m.d[1][3];
    T sf07 = m.d[1][1] * m.d[3][3] - m.d[3][1] * m.d[1][3];
    T sf08 = m.d[1][1] * m.d[3][2] - m.d[3][1] * m.d[1][2];
    T sf09 = m.d[1][0] * m.d[3][3] - m.d[3][0] * m.d[1][3];
    T sf10 = m.d[1][0] * m.d[3][2] - m.d[3][0] * m.d[1][2];
    T sf11 = m.d[1][1] * m.d[3][3] - m.d[3][1] * m.d[1][3];
    T sf12 = m.d[1][0] * m.d[3][1] - m.d[3][0] * m.d[1][1];
    T sf13 = m.d[1][2] * m.d[2][3] - m.d[2][2] * m.d[1][3];
    T sf14 = m.d[1][1] * m.d[2][3] - m.d[2][1] * m.d[1][3];
    T sf15 = m.d[1][1] * m.d[2][2] - m.d[2][1] * m.d[1][2];
    T sf16 = m.d[1][0] * m.d[2][3] - m.d[2][0] * m.d[1][3];
    T sf17 = m.d[1][0] * m.d[2][2] - m.d[2][0] * m.d[1][2];
    T sf18 = m.d[1][0] * m.d[2][1] - m.d[2][0] * m.d[1][1];

    gbMat4<T> r;
    r.d[0][0] = +(m.d[1][1] * sf00 - m.d[1][2] * sf01 + m.d[1][3] * sf02);
    r.d[1][0] = -(m.d[1][0] * sf00 - m.d[1][2] * sf03 + m.d[1][3] * sf04);
    r.d[2][0] = +(m.d[1][0] * sf01 - m.d[1][1] * sf03 + m.d[1][3] * sf05);
    r.d[3][0] = -(m.d[1][0] * sf02 - m.d[1][1] * sf04 + m.d[1][2] * sf05);

    r.d[0][1] = -(m.d[0][1] * sf00 - m.d[0][2] * sf01 + m.d[0][3] * sf02);
    r.d[1][1] = +(m.d[0][0] * sf00 - m.d[0][2] * sf03 + m.d[0][3] * sf04);
    r.d[2][1] = -(m.d[0][0] * sf01 - m.d[0][1] * sf03 + m.d[0][3] * sf05);
    r.d[3][1] = +(m.d[0][0] * sf02 - m.d[0][1] * sf04 + m.d[0][2] * sf05);

    r.d[0][2] = +(m.d[0][1] * sf06 - m.d[0][2] * sf07 + m.d[0][3] * sf08);
    r.d[1][2] = -(m.d[0][0] * sf06 - m.d[0][2] * sf09 + m.d[0][3] * sf10);
    r.d[2][2] = +(m.d[0][0] * sf11 - m.d[0][1] * sf09 + m.d[0][3] * sf12);
    r.d[3][2] = -(m.d[0][0] * sf08 - m.d[0][1] * sf10 + m.d[0][2] * sf12);

    r.d[0][3] = -(m.d[0][1] * sf13 - m.d[0][2] * sf14 + m.d[0][3] * sf15);
    r.d[1][3] = +(m.d[0][0] * sf13 - m.d[0][2] * sf16 + m.d[0][3] * sf17);
    r.d[2][3] = -(m.d[0][0] * sf14 - m.d[0][1] * sf16 + m.d[0][3] * sf18);
    r.d[3][3] = +(m.d[0][0] * sf15 - m.d[0][1] * sf17 + m.d[0][2] * sf18);

    T ood = 1.0f / (m.d[0][0] * r.d[0][0] +
                  m.d[0][1] * r.d[1][0] +
                  m.d[0][2] * r.d[2][0] +
                  m.d[0][3] * r.d[3][0]);

    r.d[0][0] *= ood;
    r.d[0][1] *= ood;
    r.d[0][2] *= ood;
    r.d[0][3] *= ood;
    r.d[1][0] *= ood;
    r.d[1][1] *= ood;
    r.d[1][2] *= ood;
    r.d[1][3] *= ood;
    r.d[2][0] *= ood;
    r.d[2][1] *= ood;
    r.d[2][2] *= ood;
    r.d[2][3] *= ood;
    r.d[3][0] *= ood;
    r.d[3][1] *= ood;
    r.d[3][2] *= ood;
    r.d[3][3] *= ood;
    return r;
}







template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_translate(const gbVec3<T>& v) {
    gbMat4<T> r;
    gb_mat4_identity(r);
    r.col[3].xyz = v;
    r.col[3].w  = 1;
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_rotate(const gbVec3<T>& v, const T angle_radians) {
    T c = gb_cos(angle_radians);
    T s = gb_sin(angle_radians);

    gbVec3<T> axis = gb_vec3_norm(v);
    gbVec3<T> t = gb_vec3_mul(axis, 1.0f-c);

    gbMat4<T> r;
    gb_mat4_identity(r);

    r.d[0][0] = c + t.x*axis.x;
    r.d[0][1] = 0 + t.x*axis.y + s*axis.z;
    r.d[0][2] = 0 + t.x*axis.z - s*axis.y;
    r.d[0][3] = 0;

    r.d[1][0] = 0 + t.y*axis.x - s*axis.z;
    r.d[1][1] = c + t.y*axis.y;
    r.d[1][2] = 0 + t.y*axis.z + s*axis.x;
    r.d[1][3] = 0;

    r.d[2][0] = 0 + t.z*axis.x + s*axis.y;
    r.d[2][1] = 0 + t.z*axis.y - s*axis.x;
    r.d[2][2] = c + t.z*axis.z;
    r.d[2][3] = 0;
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_scale(const gbVec3<T>& v) {
    gbMat4<T> r;
    gb_mat4_identity(r);
    r.e[0]  = v.x;
    r.e[5]  = v.y;
    r.e[10] = v.z;
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_scalef(const T s) {
    gbMat4<T> r;
    gb_mat4_identity(r);
    r.e[0]  = s;
    r.e[5]  = s;
    r.e[10] = s;
    return r;
}


template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_ortho2d(const T left, const T right, const T bottom, const T top) {
    gbMat4<T> r;
    gb_mat4_identity(r);

    r.d[0][0] = 2.0f / (right - left);
    r.d[1][1] = 2.0f / (top - bottom);
    r.d[2][2] = -1.0f;
    r.d[3][0] = -(right + left) / (right - left);
    r.d[3][1] = -(top + bottom) / (top - bottom);
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_ortho3d(const T left, const T right, const T bottom, const T top, const T z_near, const T z_far) {
    gbMat4<T> r;
    gb_mat4_identity(r);

    r.d[0][0] = +2.0f / (right - left);
    r.d[1][1] = +2.0f / (top - bottom);
    r.d[2][2] = -2.0f / (z_far - z_near);
    r.d[3][0] = -(right + left)   / (right - left);
    r.d[3][1] = -(top   + bottom) / (top   - bottom);
    r.d[3][2] = -(z_far + z_near) / (z_far - z_near);
    return r;
}


template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_perspective(const T fovy, const T aspect, const T z_near, const T z_far) {
    T tan_half_fovy = gb_tan(0.5f * fovy);
    gbMat4<T> r = {0};

    r.d[0][0] = 1.0f / (aspect*tan_half_fovy);
    r.d[1][1] = 1.0f / (tan_half_fovy);
    r.d[2][2] = -(z_far + z_near) / (z_far - z_near);
    r.d[2][3] = -1.0f;
    r.d[3][2] = -2.0f*z_far*z_near / (z_far - z_near);
    return r;
}

template <typename T> GB_MATH_DEF gbMat4<T> gb_mat4_perspective_directx_rh(T fovy, T aspect, T z_near, T z_far) {
    T tan_half_fovy = gb_tan(0.5f * fovy);
    gbMat4<T> out = {};
    float yscale = 1.0f / tan_half_fovy;
    out.col[0].e[0] = yscale / aspect;
    out.col[1].e[1] = yscale;
    out.col[2].e[2] = z_far / (z_near - z_far);
    out.col[2].e[3] = -1.0f;
    out.col[3].e[2] = z_near * z_far / (z_near - z_far);
    return out;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_infinite_perspective(const T fovy, const T aspect, const T z_near) {
    T range  = gb_tan(0.5f * fovy) * z_near;
    T left   = -range * aspect;
    T right  =  range * aspect;
    T bottom = -range;
    T top    =  range;
    gbMat4<T> r = {0};

    r.d[0][0] = (2.0f*z_near) / (right - left);
    r.d[1][1] = (2.0f*z_near) / (top   - bottom);
    r.d[2][2] = -1.0f;
    r.d[2][3] = -1.0f;
    r.d[3][2] = -2.0f*z_near;
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_look_at(const gbVec3<T>& eye, const gbVec3<T>& centre, const gbVec3<T>& up) {
    gbVec3<T> f = gb_vec3_sub(centre, eye);
    f = gb_vec3_norm(f);

    gbVec3<T> s = gb_vec3_cross(f, up);
    s = gb_vec3_norm(s);

    gbVec3<T> u = gb_vec3_cross(s, f);

    gbMat4<T> r;
    gb_mat4_identity(r);

    r.d[0][0] = +s.x;
    r.d[1][0] = +s.y;
    r.d[2][0] = +s.z;

    r.d[0][1] = +u.x;
    r.d[1][1] = +u.y;
    r.d[2][1] = +u.z;

    r.d[0][2] = -f.x;
    r.d[1][2] = -f.y;
    r.d[2][2] = -f.z;

    r.d[3][0] = -gb_vec3_dot(s, eye);
    r.d[3][1] = -gb_vec3_dot(u, eye);
    r.d[3][2] = +gb_vec3_dot(f, eye);
    return r;
}


/*******
// QUAT
********/

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat(const T x, const T y, const T z, const T w) 
{ gbQuat<T> q; q.x = x; q.y = y; q.z = z; q.w = w; return q; }

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_axis_angle(const gbVec3<T>& axis, const T angle_radians) {
    gbQuat<T> q;
    q.xyz = gb_vec3_norm(axis) * gb_sin(0.5f * angle_radians);
    q.w = gb_cos(0.5f * angle_radians);
    return q;
}

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_euler_angles(T pitch, T yaw, T roll) {
    /* TODO(bill): Do without multiplication, i.e. make it faster */
    gbQuat<T> p = gb_quat_axis_angle(gb_vec3(1.0f, 0.0f, 0.0f), pitch);
    gbQuat<T> y = gb_quat_axis_angle(gb_vec3(0.0f, 1.0f, 0.0f), yaw);
    gbQuat<T> r = gb_quat_axis_angle(gb_vec3(0.0f, 0.0f, 1.0f), roll);

    gbQuat<T> q = gb_quat_mul(y, p);
    q *= r;

    return q;
}

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_identity(void) { gbQuat<T> q = {0, 0, 0, 1}; return q; }

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_add(const gbQuat<T>& q0, const gbQuat<T>& q1) { gbQuat<T> r; r.xyzw = gb_vec4_add(q0.xyzw, q1.xyzw); return r; }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_sub(const gbQuat<T>& q0, const gbQuat<T>& q1) { gbQuat<T> r; r.xyzw = gb_vec4_sub(q0.xyzw, q1.xyzw); return r; }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_mul(const gbQuat<T>& q0, const gbQuat<T>& q1) {
    gbQuat<T> d;
    d.x = q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y;
    d.y = q0.w * q1.y - q0.x * q1.z + q0.y * q1.w + q0.z * q1.x;
    d.z = q0.w * q1.z + q0.x * q1.y - q0.y * q1.x + q0.z * q1.w;
    d.w = q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z;
    return d;
}
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_div(const gbQuat<T>& q0, const gbQuat<T>& q1){ return gb_quat_mul(q0, gb_quat_inverse(q1)); }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_mulf(const gbQuat<T>& q0, const T s) { gbQuat<T> r; r.xyzw = gb_vec4_mul(q0.xyzw, s); return r; }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_divf(const gbQuat<T>& q0, const T s) { gbQuat<T> r; r.xyzw = gb_vec4_div(q0.xyzw, s); return r; }

template<typename T> GB_MATH_DEF void gb_quat_addeq(gbQuat<T>& d, const gbQuat<T>& q) { gb_vec4_addeq(d.xyzw, q.xyzw); }
template<typename T> GB_MATH_DEF void gb_quat_subeq(gbQuat<T>& d, const gbQuat<T>& q) { gb_vec4_subeq(d.xyzw, q.xyzw); }
template<typename T> GB_MATH_DEF void gb_quat_muleq(gbQuat<T>& d, const gbQuat<T>& q) { d = gb_quat_mul(d, q); }
template<typename T> GB_MATH_DEF void gb_quat_diveq(gbQuat<T>& d, const gbQuat<T>& q) { d = gb_quat_div(d, q); }

template<typename T> GB_MATH_DEF void gb_quat_muleqf(gbQuat<T>& d, const T s) { gb_vec4_muleq(d.xyzw, s); }
template<typename T> GB_MATH_DEF void gb_quat_diveqf(gbQuat<T>& d, const T s) { gb_vec4_diveq(d.xyzw, s); }

template<typename T> GB_MATH_DEF T gb_quat_dot(const gbQuat<T>& q0, const gbQuat<T>& q1) { return gb_vec3_dot(q0.xyz, q1.xyz) + q0.w*q1.w; }
template<typename T> GB_MATH_DEF T gb_quat_mag(const gbQuat<T>& q)                       { return gb_sqrt(gb_quat_dot(q, q)); }

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_norm(const gbQuat<T>& q)    { return gb_quat_divf(q, gb_quat_mag(q)); }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_conj(const gbQuat<T>& q)    { return { -q.x, -q.y, -q.z, q.w }; }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_inverse(const gbQuat<T>& q) { gbQuat<T> r = gb_quat_conj(q); gb_quat_diveqf(r, gb_quat_dot(q, q)); return r; }

template<typename T> GB_MATH_DEF gbVec3<T> gb_quat_axis(const gbQuat<T>& q) { return gb_vec3_div(gb_quat_norm(q).xyz, gb_sin(gb_arccos(q.w))); }
template<typename T> 
GB_MATH_DEF T gb_quat_angle(const gbQuat<T>& q) {
    T mag = gb_quat_mag(q);
    T c = q.w * (1.0f/mag);
    T angle = 2.0f*gb_arccos(c);
    return angle;
}

template<typename T> GB_MATH_DEF T gb_quat_pitch(const gbQuat<T>& q) { return gb_arctan2(2.0f*q.y*q.z + q.w*q.x, q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z); }
template<typename T> GB_MATH_DEF T gb_quat_yaw  (const gbQuat<T>& q) { return gb_arcsin(-2.0f*(q.x*q.z - q.w*q.y)); }
template<typename T> GB_MATH_DEF T gb_quat_roll (const gbQuat<T>& q) { return gb_arctan2(2.0f*q.x*q.y + q.z*q.w, q.x*q.x + q.w*q.w - q.y*q.y - q.z*q.z); }

/* NOTE(bill): Rotate v by q */
template<typename T> GB_MATH_DEF gbVec3<T> gb_quat_rotate_vec3(const gbQuat<T>& q, const gbVec3<T>& v) {
    /* gbVec3<T> t = 2.0f * cross(q.xyz, v);
     * *d = q.w*t + v + cross(q.xyz, t);
     */
    gbVec3<T> t = gb_vec3_cross(q.xyz, v);
    gb_vec3_muleq(t, 2.0f);

    gbVec3<T> p = gb_vec3_cross(q.xyz, t);

    gbVec3<T> r = gb_vec3_mul(t, q.w);
    gb_vec3_addeq(r, v);
    gb_vec3_addeq(r, p);
    return r;
}

template<typename T> GB_MATH_DEF gbMat4<T> gb_mat4_from_quat(const gbQuat<T>& q) {
    T xx, yy, zz,
      xy, xz, yz,
      wx, wy, wz;

    gbQuat<T> a = gb_quat_norm(q);
    xx = a.x*a.x; yy = a.y*a.y; zz = a.z*a.z;
    xy = a.x*a.y; xz = a.x*a.z; yz = a.y*a.z;
    wx = a.w*a.x; wy = a.w*a.y; wz = a.w*a.z;

    gbMat4<T> r;
    gb_mat4_identity(r);

    r.d[0][0] = 1.0f - 2.0f*(yy + zz);
    r.d[0][1] =        2.0f*(xy + wz);
    r.d[0][2] =        2.0f*(xz - wy);

    r.d[1][0] =        2.0f*(xy - wz);
    r.d[1][1] = 1.0f - 2.0f*(xx + zz);
    r.d[1][2] =        2.0f*(yz + wx);

    r.d[2][0] =        2.0f*(xz + wy);
    r.d[2][1] =        2.0f*(yz - wx);
    r.d[2][2] = 1.0f - 2.0f*(xx + yy);
    return r;
}

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_from_mat4(const gbMat4<T>& m) {
    int biggest_index = 0;

    T four_x_squared_minus_1 = m.d[0][0] - m.d[1][1] - m.d[2][2];
    T four_y_squared_minus_1 = m.d[1][1] - m.d[0][0] - m.d[2][2];
    T four_z_squared_minus_1 = m.d[2][2] - m.d[0][0] - m.d[1][1];
    T four_w_squared_minus_1 = m.d[0][0] + m.d[1][1] + m.d[2][2];

    T four_biggest_squared_minus_1 = four_w_squared_minus_1;
    if (four_x_squared_minus_1 > four_biggest_squared_minus_1) {
        four_biggest_squared_minus_1 = four_x_squared_minus_1;
        biggest_index = 1;
    }
    if (four_y_squared_minus_1 > four_biggest_squared_minus_1) {
        four_biggest_squared_minus_1 = four_y_squared_minus_1;
        biggest_index = 2;
    }
    if (four_z_squared_minus_1 > four_biggest_squared_minus_1) {
        four_biggest_squared_minus_1 = four_z_squared_minus_1;
        biggest_index = 3;
    }

    T biggest_value = gb_sqrt(four_biggest_squared_minus_1 + 1.0f) * 0.5f;
    T mult = 0.25f / biggest_value;
    gbQuat<T> r;

    switch (biggest_index) {
    case 0:
        r.w = biggest_value;
        r.x = (m.d[1][2] - m.d[2][1]) * mult;
        r.y = (m.d[2][0] - m.d[0][2]) * mult;
        r.z = (m.d[0][1] - m.d[1][0]) * mult;
        break;
    case 1:
        r.w = (m.d[1][2] - m.d[2][1]) * mult;
        r.x = biggest_value;
        r.y = (m.d[0][1] + m.d[1][0]) * mult;
        r.z = (m.d[2][0] + m.d[0][2]) * mult;
        break;
    case 2:
        r.w = (m.d[2][0] - m.d[0][2]) * mult;
        r.x = (m.d[0][1] + m.d[1][0]) * mult;
        r.y = biggest_value;
        r.z = (m.d[1][2] + m.d[2][1]) * mult;
        break;
    case 3:
        r.w = (m.d[0][1] - m.d[1][0]) * mult;
        r.x = (m.d[2][0] + m.d[0][2]) * mult;
        r.y = (m.d[1][2] + m.d[2][1]) * mult;
        r.z = biggest_value;
        break;
    default:
        /* NOTE(bill): This shouldn't fucking happen!!! */
        assert(false);
        break;
    }
    return r;
}





/*******
// INTERPOLATIONS
********/

template<typename T> GB_MATH_DEF T gb_lerp         (const T a, const T b, const T t) { return a*(1.0f-t) + b*t; }
template<typename T> GB_MATH_DEF T gb_unlerp       (const T t, const T a, const T b) { return (t-a)/(b-a); }
template<typename T> GB_MATH_DEF T gb_smooth_step  (const T a, const T b, const T t) { T x = (t - a)/(b - a); return x*x*(3.0f - 2.0f*x); }
template<typename T> GB_MATH_DEF T gb_smoother_step(const T a, const T b, const T t) { T x = (t - a)/(b - a); return x*x*x*(x*(6.0f*x - 15.0f) + 10.0f); }

#define GB_LERP_SCOPE(a, b, t) a + (b - a) * t
template<typename T> GB_MATH_DEF gbVec2<T> gb_vec2_lerp(const gbVec2<T>& a, const gbVec2<T>& b, const T t) { return GB_LERP_SCOPE(a, b, t); }
template<typename T> GB_MATH_DEF gbVec3<T> gb_vec3_lerp(const gbVec3<T>& a, const gbVec3<T>& b, const T t) { return GB_LERP_SCOPE(a, b, t); }
template<typename T> GB_MATH_DEF gbVec4<T> gb_vec4_lerp(const gbVec4<T>& a, const gbVec4<T>& b, const T t) { return GB_LERP_SCOPE(a, b, t); }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_lerp(const gbQuat<T>& a, const gbQuat<T>& b, const T t) { return GB_LERP_SCOPE(a, b, t); }
#undef GB_LERP_SCOPE

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_nlerp(const gbQuat<T>& a, const gbQuat<T>& b, T t) { return gb_quat_norm(gb_quat_lerp(a, b, t)); }
template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_slerp(const gbQuat<T>& a, const gbQuat<T>& b, const T t) {
    gbQuat<T> z = b;
    T cos_theta = gb_quat_dot(a, b);

    if (cos_theta < 0.0f) {
        z = gb_quat(-b.x, -b.y, -b.z, -b.w);
        cos_theta = -cos_theta;
    }

    gbQuat<T> r;
    if (cos_theta > 1.0f) {
        /* NOTE(bill): Use lerp not nlerp as it's not a real angle or they are not normalized */
        r = gb_quat_lerp(a, b, t);
    }

    T angle = gb_arccos(cos_theta);

    T s1 = gb_sin((1.0f - t)*angle);
    T s0 = gb_sin(t*angle);
    T is = 1.0f/gb_sin(angle);
    gbQuat<T> x = gb_quat_mulf(a, s1);
    gbQuat<T> y = gb_quat_mulf(z, s0);
    r = gb_quat_add(x, y);
    gb_quat_muleqf(r, is);
    return r;
}

template<typename T> GB_MATH_DEF gbQuat<T> gb_quat_slerp_approx(const gbQuat<T>& a, const gbQuat<T>& b, const T t) {
    /* NOTE(bill): Derived by taylor expanding the geometric interpolation equation
     *             Even works okay for nearly anti-parallel versors!!!
     */
    /* NOTE(bill): Extra interations cannot be used as they require angle^4 which is not worth it to approximate */
    T tp = t + (1.0f - gb_quat_dot(a, b))/3.0f * t*(-2.0f*t*t + 3.0f*t - 1.0f);
    return gb_quat_nlerp(a, b, tp);
}

template<typename T> 
GB_MATH_DEF gbQuat<T>  gb_quat_nquad(const gbQuat<T>& p, const gbQuat<T>& a, const gbQuat<T>& b, const gbQuat<T>& q, const T t) {
    gbQuat<T> x = gb_quat_nlerp(p, q, t);
    gbQuat<T> y = gb_quat_nlerp(a, b, t);
    return gb_quat_nlerp(x, y, 2.0f*t*(1.0f-t));
}

template<typename T> 
GB_MATH_DEF gbQuat<T> gb_quat_squad(const gbQuat<T>& p, const gbQuat<T>& a, const gbQuat<T>& b, const gbQuat<T>& q, const T t) {
    gbQuat<T> x = gb_quat_slerp(p, q, t);
    gbQuat<T> y = gb_quat_slerp(a, b, t);
    return gb_quat_slerp(x, y, 2.0f*t*(1.0f-t));
}

template<typename T> 
GB_MATH_DEF gbQuat<T> gb_quat_squad_approx(const gbQuat<T>& p, const gbQuat<T>& a, const gbQuat<T>& b, const gbQuat<T>& q, const T t) {
    gbQuat<T> x = gb_quat_slerp_approx(p, q, t);
    gbQuat<T> y = gb_quat_slerp_approx(a, b, t);
    return gb_quat_slerp_approx(x, y, 2.0f*t*(1.0f-t));
}



/*******
// RECTS
********/

template<typename T> 
GB_MATH_DEF gbRect2<T> gb_rect2(const gbVec2<T>& pos, const gbVec2<T>& dim) {
    gbRect2<T> r;
    r.pos = pos;
    r.dim = dim;
    return r;
}

template<typename T> 
GB_MATH_DEF gbRect3<T> gb_rect3(const gbVec3<T>& pos, const gbVec3<T>& dim) {
    gbRect3<T> r;
    r.pos = pos;
    r.dim = dim;
    return r;
}

template<typename T> GB_MATH_DEF int gb_rect2_contains(const gbRect2<T>& a, const T x, const T y) {
    T min_x = gb_min(a.pos.x, a.pos.x+a.dim.x);
    T max_x = gb_max(a.pos.x, a.pos.x+a.dim.x);
    T min_y = gb_min(a.pos.y, a.pos.y+a.dim.y);
    T max_y = gb_max(a.pos.y, a.pos.y+a.dim.y);
    int result = (x >= min_x) & (x < max_x) & (y >= min_y) & (y < max_y);
    return result;
}

template<typename T> 
GB_MATH_DEF int gb_rect2_contains_vec2(const gbRect2<T>& a, const gbVec2<T>& p) { return gb_rect2_contains(a, p.x, p.y); }

template<typename T> 
GB_MATH_DEF int gb_rect2_intersects(const gbRect2<T>& a, const gbRect2<T>& b) {
    gbRect2<T> r = {0};
    return gb_rect2_intersection_result(a, b, r);
}

template<typename T> 
GB_MATH_DEF int gb_rect2_intersection_result(const gbRect2<T>& a, const gbRect2<T>& b, gbRect2<T>& intersection) {
    T a_min_x = gb_min(a.pos.x, a.pos.x+a.dim.x);
    T a_max_x = gb_max(a.pos.x, a.pos.x+a.dim.x);
    T a_min_y = gb_min(a.pos.y, a.pos.y+a.dim.y);
    T a_max_y = gb_max(a.pos.y, a.pos.y+a.dim.y);

    T b_min_x = gb_min(b.pos.x, b.pos.x+b.dim.x);
    T b_max_x = gb_max(b.pos.x, b.pos.x+b.dim.x);
    T b_min_y = gb_min(b.pos.y, b.pos.y+b.dim.y);
    T b_max_y = gb_max(b.pos.y, b.pos.y+b.dim.y);

    T x0 = gb_max(a_min_x, b_min_x);
    T y0 = gb_max(a_min_y, b_min_y);
    T x1 = gb_min(a_max_x, b_max_x);
    T y1 = gb_min(a_max_y, b_max_y);

    if ((x0 < x1) && (y0 < y1)) {
        intersection = gb_rect2(gb_vec2(x0, y0), gb_vec2(x1-x0, y1-y0));
        return 1;
    } else {
        intersection = {0};
        return 0;
    }
}



/*******
// HASHING
********/

#ifndef	GB_MURMUR64_DEFAULT_SEED
#define GB_MURMUR64_DEFAULT_SEED 0x9747b28c
#endif
/* Hashing */
GB_MATH_DEF gb_math_u64 gb_hash_murmur64(void const *key, size_t num_bytes, gb_math_u64 seed);

/* Random */
/* TODO(bill): Use a generator for the random numbers */
GB_MATH_DEF float gb_random_range_float(float min_inc, float max_inc);
GB_MATH_DEF int   gb_random_range_int  (int min_inc, int max_inc);
GB_MATH_DEF float gb_random01          (void);




/* TODO(bill): How should I apply GB_MATH_DEF to these operator overloads? */

template<typename T> inline bool      operator==(const gbVec2<T>& a, const gbVec2<T>& b) { return (a.x == b.x) && (a.y == b.y); }
template<typename T> inline bool      operator!=(const gbVec2<T>& a, const gbVec2<T>& b) { return !operator==(a, b); }

template<typename T> inline gbVec2<T> operator+(const gbVec2<T>& a) { return a; }
template<typename T> inline gbVec2<T> operator-(const gbVec2<T>& a) { return { -a.x, -a.y }; }

template<typename T> inline gbVec2<T> operator+(gbVec2<T> a, gbVec2<T> b) { return gb_vec2_add(a, b); }
template<typename T> inline gbVec2<T> operator-(gbVec2<T> a, gbVec2<T> b) { return gb_vec2_sub(a, b); }

template<typename T> inline gbVec2<T> operator*(gbVec2<T> a, T scalar) { return gb_vec2_mul(a, scalar); }
template<typename T> inline gbVec2<T> operator*(T scalar, gbVec2<T> a) { return operator*(a, scalar); }

template<typename T> inline gbVec2<T> operator/(gbVec2<T> a, T scalar) { gbVec2<T> r = { a.x / scalar, a.x / scalar }; return r; }
template<typename T> inline gbVec2<T> operator/(T scalar, gbVec2<T> a) { gbVec2<T> r = { scalar / a.x, scalar / a.y }; return r; }

/* Hadamard Product */
template<typename T> inline gbVec2<T> operator*(gbVec2<T> a, gbVec2<T> b) { gbVec2<T> r = {a.x*b.x, a.y*b.y}; return r; }
template<typename T> inline gbVec2<T> operator/(gbVec2<T> a, gbVec2<T> b) { gbVec2<T> r = {a.x/b.x, a.y/b.y}; return r; }

template<typename T> inline gbVec2<T> &operator+=(gbVec2<T> &a, gbVec2<T> b)       { return (a = a + b); }
template<typename T> inline gbVec2<T> &operator-=(gbVec2<T> &a, gbVec2<T> b)       { return (a = a - b); }
template<typename T> inline gbVec2<T> &operator*=(gbVec2<T> &a, T scalar) { return (a = a * scalar); }
template<typename T> inline gbVec2<T> &operator/=(gbVec2<T> &a, T scalar) { return (a = a / scalar); }


template<typename T> inline bool      operator==(gbVec3<T> a, gbVec3<T> b) { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }
template<typename T> inline bool      operator!=(gbVec3<T> a, gbVec3<T> b) { return !operator==(a, b); }

template<typename T> inline gbVec3<T> operator+(gbVec3<T> a) { return a; }
template<typename T> inline gbVec3<T> operator-(gbVec3<T> a) { gbVec3<T> r = {-a.x, -a.y, -a.z}; return r; }

template<typename T> inline gbVec3<T> operator+(gbVec3<T> a, gbVec3<T> b) { return gb_vec3_add(a, b); }
template<typename T> inline gbVec3<T> operator-(gbVec3<T> a, gbVec3<T> b) { return gb_vec3_sub(a, b); }

template<typename T> inline gbVec3<T> operator*(gbVec3<T> a, T scalar) { return gb_vec3_mul(a, scalar); }
template<typename T> inline gbVec3<T> operator*(T scalar, gbVec3<T> a) { return operator*(a, scalar); }

template<typename T> inline gbVec3<T> operator/(gbVec3<T> a, T scalar) { return operator*(a, 1.0f/scalar); }
template<typename T> inline gbVec3<T> operator/(T scalar, gbVec3<T> a) { gbVec3<T> r = { scalar / a.x, scalar / a.y, scalar / a.z }; return r; }

/* Hadamard Product */
template<typename T> inline gbVec3<T> operator*(gbVec3<T> a, gbVec3<T> b) { gbVec3<T> r = {a.x*b.x, a.y*b.y, a.z*b.z}; return r; }
template<typename T> inline gbVec3<T> operator/(gbVec3<T> a, gbVec3<T> b) { gbVec3<T> r = {a.x/b.x, a.y/b.y, a.z/b.z}; return r; }

template<typename T> inline gbVec3<T> &operator+=(gbVec3<T> &a, gbVec3<T> b)       { return (a = a + b); }
template<typename T> inline gbVec3<T> &operator-=(gbVec3<T> &a, gbVec3<T> b)       { return (a = a - b); }
template<typename T> inline gbVec3<T> &operator*=(gbVec3<T> &a, T scalar) { return (a = a * scalar); }
template<typename T> inline gbVec3<T> &operator/=(gbVec3<T> &a, T scalar) { return (a = a / scalar); }


template<typename T> inline bool      operator==(gbVec4<T> a, gbVec4<T> b) { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w); }
template<typename T> inline bool      operator!=(gbVec4<T> a, gbVec4<T> b) { return !operator==(a, b); }

template<typename T> inline gbVec4<T> operator+(gbVec4<T> a) { return a; }
template<typename T> inline gbVec4<T> operator-(gbVec4<T> a) { gbVec4<T> r = {-a.x, -a.y, -a.z, -a.w}; return r; }

template<typename T> inline gbVec4<T> operator+(gbVec4<T> a, gbVec4<T> b) { return gb_vec4_add(a, b); }
template<typename T> inline gbVec4<T> operator-(gbVec4<T> a, gbVec4<T> b) { return gb_vec4_sub(a, b); }

template<typename T> inline gbVec4<T> operator*(gbVec4<T> a, T scalar) { return gb_vec4_mul(a, scalar); }
template<typename T> inline gbVec4<T> operator*(T scalar, gbVec4<T> a) { return operator*(a, scalar); }

template<typename T> inline gbVec4<T> operator/(gbVec4<T> a, T scalar) { return operator*(a, 1.0f/scalar); }
template<typename T> inline gbVec4<T> operator/(T scalar, gbVec4<T> a) { gbVec3<T> r = { scalar / a.x, scalar / a.y, scalar / a.z, scalar / a.w }; return r; }

/* Hadamard Product */
template<typename T> inline gbVec4<T> operator*(gbVec4<T> a, gbVec4<T> b) { gbVec4<T> r = {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; return r; }
template<typename T> inline gbVec4<T> operator/(gbVec4<T> a, gbVec4<T> b) { gbVec4<T> r = {a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w}; return r; }

template<typename T> inline gbVec4<T> &operator+=(gbVec4<T> &a, gbVec4<T> b)       { return (a = a + b); }
template<typename T> inline gbVec4<T> &operator-=(gbVec4<T> &a, gbVec4<T> b)       { return (a = a - b); }
template<typename T> inline gbVec4<T> &operator*=(gbVec4<T> &a, T scalar) { return (a = a * scalar); }
template<typename T> inline gbVec4<T> &operator/=(gbVec4<T> &a, T scalar) { return (a = a / scalar); }


template<typename T> inline gbMat2<T> operator+(gbMat2<T> const &a, gbMat2<T> const &b) {
    int i, j;
    gbMat2<T> r = {0};
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 2; i++)
            r.e[2*j+i] = a.e[2*j+i] + b.e[2*j+i];
    }
    return r;
}

template<typename T> inline gbMat2<T> operator-(gbMat2<T> const &a, gbMat2<T> const &b) {
    int i, j;
    gbMat2<T> r = {0};
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 2; i++)
            r.e[2*j+i] = a.e[2*j+i] - b.e[2*j+i];
    }
    return r;
}

template<typename T> inline gbMat2<T> operator*(const gbMat2<T>& a, const gbMat2<T>& b) { return gb_mat2_mul(a, b); }
template<typename T> inline gbVec2<T> operator*(const gbMat2<T>& a, const gbVec2<T>& v) { return gb_mat2_mul_vec2(a, v); }
template<typename T> inline gbMat2<T> operator*(const gbMat2<T>& a, const T scalar) {
    gbMat2<T> r = {0};
    int i;
    for (i = 0; i < 2*2; i++) r.e[i] = a.e[i] * scalar;
    return r;
}
template<typename T> inline gbMat2<T> operator*(T scalar, gbMat2<T> const &a) { return operator*(a, scalar); }
template<typename T> inline gbMat2<T> operator/(gbMat2<T> const &a, T scalar) { return operator*(a, 1.0f/scalar); }

template<typename T> inline gbMat2<T>& operator+=(gbMat2<T>& a, gbMat2<T> const &b) { return (a = a + b); }
template<typename T> inline gbMat2<T>& operator-=(gbMat2<T>& a, gbMat2<T> const &b) { return (a = a - b); }
template<typename T> inline gbMat2<T>& operator*=(gbMat2<T>& a, gbMat2<T> const &b) { return (a = a * b); }



template<typename T> inline gbMat3<T> operator+(gbMat3<T> const &a, gbMat3<T> const &b) {
    int i, j;
    gbMat3<T> r = {0};
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++)
            r.e[3*j+i] = a.e[3*j+i] + b.e[3*j+i];
    }
    return r;
}

template<typename T> inline gbMat3<T> operator-(gbMat3<T> const &a, gbMat3<T> const &b) {
    int i, j;
    gbMat3<T> r = {0};
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++)
            r.e[3*j+i] = a.e[3*j+i] - b.e[3*j+i];
    }
    return r;
}

template<typename T> inline gbMat3<T> operator*(gbMat3<T> const &a, gbMat3<T> const &b) { return gb_mat3_mul(a, b); }
template<typename T> inline gbVec3<T> operator*(gbMat3<T> const &a, gbVec3<T> v) { return gb_mat3_mul_vec3(a, v); } 
template<typename T> inline gbMat3<T> operator*(gbMat3<T> const &a, T scalar) {
    gbMat3<T> r = {0};
    int i;
    for (i = 0; i < 3*3; i++) r.e[i] = a.e[i] * scalar;
    return r;
}
template<typename T> inline gbMat3<T> operator*(T scalar, gbMat3<T> const &a) { return operator*(a, scalar); }
template<typename T> inline gbMat3<T> operator/(gbMat3<T> const &a, T scalar) { return operator*(a, 1.0f/scalar); }

template<typename T> inline gbMat3<T>& operator+=(gbMat3<T>& a, gbMat3<T> const &b) { return (a = a + b); }
template<typename T> inline gbMat3<T>& operator-=(gbMat3<T>& a, gbMat3<T> const &b) { return (a = a - b); }
template<typename T> inline gbMat3<T>& operator*=(gbMat3<T>& a, gbMat3<T> const &b) { return (a = a * b); }



template<typename T> inline gbMat4<T> operator+(gbMat4<T> const &a, gbMat4<T> const &b) {
    int i, j;
    gbMat4<T> r = {0};
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++)
            r.e[4*j+i] = a.e[4*j+i] + b.e[4*j+i];
    }
    return r;
}

template<typename T> inline gbMat4<T> operator-(gbMat4<T> const &a, gbMat4<T> const &b) {
    int i, j;
    gbMat4<T> r = {0};
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++)
            r.e[4*j+i] = a.e[4*j+i] - b.e[4*j+i];
    }
    return r;
}

template<typename T> inline gbMat4<T> operator*(const gbMat4<T>& a, const gbMat4<T>& b) { return gb_mat4_mul(a, b); }
template<typename T> inline gbVec4<T> operator*(const gbMat4<T>& a, const gbVec4<T>& v) { return gb_mat4_mul_vec4(a, v); }
template<typename T> inline gbMat4<T> operator*(const gbMat4<T>& a, const T scalar) {
    gbMat4<T> r = {0};
    int i;
    for (i = 0; i < 4*4; i++) r.e[i] = a.e[i] * scalar;
    return r;
}
template<typename T> inline gbMat4<T> operator*(T scalar, gbMat4<T> const &a) { return operator*(a, scalar); }
template<typename T> inline gbMat4<T> operator/(gbMat4<T> const &a, T scalar) { return operator*(a, 1.0f/scalar); }

template<typename T> inline gbMat4<T>& operator+=(gbMat4<T> &a, gbMat4<T> const &b) { return (a = a + b); }
template<typename T> inline gbMat4<T>& operator-=(gbMat4<T> &a, gbMat4<T> const &b) { return (a = a - b); }
template<typename T> inline gbMat4<T>& operator*=(gbMat4<T> &a, gbMat4<T> const &b) { return (a = a * b); }



template<typename T> inline bool operator==(const gbQuat<T>& a, const gbQuat<T>& b) { return a.xyzw == b.xyzw; }
template<typename T> inline bool operator!=(const gbQuat<T>& a, const gbQuat<T>& b) { return !operator==(a, b); }

template<typename T> inline gbQuat<T> operator+(const gbQuat<T>& q) { return q; }
template<typename T> inline gbQuat<T> operator-(const gbQuat<T>& q) { return gb_quat(-q.x, -q.y, -q.z, -q.w); }

template<typename T> inline gbQuat<T> operator+(gbQuat<T> a, gbQuat<T> b) { return gb_quat_add(a, b); }
template<typename T> inline gbQuat<T> operator-(gbQuat<T> a, gbQuat<T> b) { return gb_quat_sub(a, b); }

template<typename T> inline gbQuat<T> operator*(gbQuat<T> a, gbQuat<T> b)  { return gb_quat_mul(a, b); }
template<typename T> inline gbQuat<T> operator*(gbQuat<T> q, T s) { return gb_quat_mulf(q, s); }
template<typename T> inline gbQuat<T> operator*(T s, gbQuat<T> q) { return operator*(q, s); }
template<typename T> inline gbQuat<T> operator/(gbQuat<T> q, T s) { return gb_quat_divf(q, s); }
template<typename T> inline gbQuat<T> operator/(T s, gbQuat<T> a) { gbVec3<T> r = { s / a.x, s / a.y, s / a.z, s / a.w }; return r; }

template<typename T> inline gbQuat<T> &operator+=(gbQuat<T>& a, gbQuat<T> b) { gb_quat_addeq(a, b); return a; }
template<typename T> inline gbQuat<T> &operator-=(gbQuat<T> &a, gbQuat<T> b) { gb_quat_subeq(a, b); return a; }
template<typename T> inline gbQuat<T> &operator*=(gbQuat<T> &a, gbQuat<T> b) { gb_quat_muleq(a, b); return a; }
template<typename T> inline gbQuat<T> &operator/=(gbQuat<T> &a, gbQuat<T> b) { gb_quat_diveq(a, b); return a; }

template<typename T> inline gbQuat<T> &operator*=(gbQuat<T> &a, T b) { gb_quat_muleqf(a, b); return a; }
template<typename T> inline gbQuat<T> &operator/=(gbQuat<T> &a, T b) { gb_quat_diveqf(a, b); return a; }

/* Rotate v by a */
template<typename T> inline gbVec3<T> operator*(const gbQuat<T>& q, const gbVec3<T>& v) { return gb_quat_rotate_vec3(q, v); }





#endif /* GB_MATH_INCLUDE_GB_MATH_H */

/****************************************************************
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 * Implementation
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 ****************************************************************/

#if defined(GB_MATH_IMPLEMENTATION) && !defined(GB_MATH_IMPLEMENTATION_DONE)
#define GB_MATH_IMPLEMENTATION_DONE

 #if (defined(__GCC__) || defined(__GNUC__)) && !defined(__clang__)
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wattributes"
 #pragma GCC diagnostic ignored "-Wmissing-braces"
 #elif __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wattributes"
 #pragma clang diagnostic ignored "-Wmissing-braces"
 #endif


/* NOTE(bill): To remove the need for memcpy */
static void gb__memcpy_4byte(void *dest, void const *src, size_t size) {
    size_t i;
    unsigned int *d, *s;
    d = (unsigned int *)dest;
    s = (unsigned int *)src;
    for (i = 0; i < size/4; i++) {
        *d++ = *s++;
    }
}


float gb_to_radians(float degrees) { return degrees * GB_MATH_TAU / 360.0f; }
float gb_to_degrees(float radians) { return radians * 360.0f / GB_MATH_TAU; }

float gb_angle_diff(float radians_a, float radians_b) {
    float delta = gb_mod(radians_b-radians_a, GB_MATH_TAU);
    delta = gb_mod(delta + 1.5f*GB_MATH_TAU, GB_MATH_TAU);
    delta -= 0.5f*GB_MATH_TAU;
    return delta;
}

float gb_copy_sign(const float x, const float y) {
    int ix, iy;
    ix = *(int *)&x;
    iy = *(int *)&y;

    ix &= 0x7fffffff;
    ix |= iy & 0x80000000;
    return *(float *)&ix;
}

float gb_remainder(const float x, const float y) {
    return x - (gb_round(x/y)*y);
}

float gb_mod(const float x, float y) {
    float result;
    y = gb_abs(y);
    result = gb_remainder(gb_abs(x), y);
    if (gb_sign(result)) result += y;
    return gb_copy_sign(result, x);
}


float gb_quake_rsqrt(const float a) {
    union {
        int i;
        float f;
    } t;
    float x2;
    float const three_halfs = 1.5f;

    x2 = a * 0.5f;
    t.f = a;
    t.i = 0x5f375a86 - (t.i >> 1);                /* What the fuck? */
    t.f = t.f * (three_halfs - (x2 * t.f * t.f)); /* 1st iteration */
    t.f = t.f * (three_halfs - (x2 * t.f * t.f)); /* 2nd iteration, this can be removed */

    return t.f;
}


#if defined(GB_MATH_NO_MATH_H)
#if defined(_MSC_VER)

    float gb_rsqrt(float a) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(a))); }
    float gb_sqrt(float a)  { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a))); };
    float
    gb_sin(float a)
    {
        static float const a0 = +1.91059300966915117e-31f;
        static float const a1 = +1.00086760103908896f;
        static float const a2 = -1.21276126894734565e-2f;
        static float const a3 = -1.38078780785773762e-1f;
        static float const a4 = -2.67353392911981221e-2f;
        static float const a5 = +2.08026600266304389e-2f;
        static float const a6 = -3.03996055049204407e-3f;
        static float const a7 = +1.38235642404333740e-4f;
        return a0 + a*(a1 + a*(a2 + a*(a3 + a*(a4 + a*(a5 + a*(a6 + a*a7))))));
    }
    float
    gb_cos(float a)
    {
        static float const a0 = +1.00238601909309722f;
        static float const a1 = -3.81919947353040024e-2f;
        static float const a2 = -3.94382342128062756e-1f;
        static float const a3 = -1.18134036025221444e-1f;
        static float const a4 = +1.07123798512170878e-1f;
        static float const a5 = -1.86637164165180873e-2f;
        static float const a6 = +9.90140908664079833e-4f;
        static float const a7 = -5.23022132118824778e-14f;
        return a0 + a*(a1 + a*(a2 + a*(a3 + a*(a4 + a*(a5 + a*(a6 + a*a7))))));
    }

    float
    gb_tan(float radians)
    {
        float rr = radians*radians;
        float a = 9.5168091e-03f;
        a *= rr;
        a += 2.900525e-03f;
        a *= rr;
        a += 2.45650893e-02f;
        a *= rr;
        a += 5.33740603e-02f;
        a *= rr;
        a += 1.333923995e-01f;
        a *= rr;
        a += 3.333314036e-01f;
        a *= rr;
        a += 1.0f;
        a *= radians;
        return a;
    }

    float gb_arcsin(float a) { return gb_arctan2(a, gb_sqrt((1.0f + a) * (1.0f - a))); }
    float gb_arccos(float a) { return gb_arctan2(gb_sqrt((1.0f + a) * (1.0 - a)), a); }

    float
    gb_arctan(float a)
    {
        float u  = a*a;
        float u2 = u*u;
        float u3 = u2*u;
        float u4 = u3*u;
        float f  = 1.0f+0.33288950512027f*u-0.08467922817644f*u2+0.03252232640125f*u3-0.00749305860992f*u4;
        return a/f;
    }

    float
    gb_arctan2(float y, float x)
    {
        if (gb_abs(x) > gb_abs(y)) {
            float a = gb_arctan(y/x);
            if (x > 0.0f)
                return a;
            else
                return y > 0.0f ? a+GB_MATH_TAU_OVER_2:a-GB_MATH_TAU_OVER_2;
        } else {
            float a = gb_arctan(x/y);
            if (x > 0.0f)
                return y > 0.0f ? GB_MATH_TAU_OVER_4-a:-GB_MATH_TAU_OVER_4-a;
            else
                return y > 0.0f ? GB_MATH_TAU_OVER_4+a:-GB_MATH_TAU_OVER_4+a;
        }
    }

    float
    gb_exp(float a)
    {
        union { float f; int i; } u, v;
        u.i = (int)(6051102 * a + 1056478197);
        v.i = (int)(1056478197 - 6051102 * a);
        return u.f / v.f;
    }

    float
    gb_log(float a)
    {
        union { float f; int i; } u = {a};
        return (u.i - 1064866805) * 8.262958405176314e-8f; /* 1 / 12102203.0; */
    }

    float
    gb_pow(float a, float b)
    {
        int flipped = 0, e;
        float f, r = 1.0f;
        if (b < 0) {
            flipped = 1;
            b = -b;
        }

        e = (int)b;
        f = gb_exp(b - e);

        while (e) {
            if (e & 1) r *= a;
            a *= a;
            e >>= 1;
        }

        r *= f;
        return flipped ? 1.0f/r : r;
    }

#else

    float gb_rsqrt(float a)            { return 1.0f/__builtin_sqrt(a); }
    float gb_sqrt(float a)             { return __builtin_sqrt(a); }
    float gb_sin(float radians)        { return __builtin_sinf(radians); }
    float gb_cos(float radians)        { return __builtin_cosf(radians); }
    float gb_tan(float radians)        { return __builtin_tanf(radians); }
    float gb_arcsin(float a)           { return __builtin_asinf(a); }
    float gb_arccos(float a)           { return __builtin_acosf(a); }
    float gb_arctan(float a)           { return __builtin_atanf(a); }
    float gb_arctan2(float y, float x) { return __builtin_atan2f(y, x); }


    float gb_exp(float x)  { return __builtin_expf(x); }
    float gb_log(float x)  { return __builtin_logf(x); }

    // TODO(bill): Should this be gb_exp(y * gb_log(x)) ???
    float gb_pow(float x, float y) { return __builtin_powf(x, y); }

#endif

#else
    float gb_rsqrt(float a)            { return 1.0f/sqrtf(a); }
    float gb_sqrt(float a)             { return sqrtf(a); };
    float gb_sin(float radians)        { return sinf(radians); };
    float gb_cos(float radians)        { return cosf(radians); };
    float gb_tan(float radians)        { return tanf(radians); };
    float gb_arcsin(float a)           { return asinf(a); };
    float gb_arccos(float a)           { return acosf(a); };
    float gb_arctan(float a)           { return atanf(a); };
    float gb_arctan2(float y, float x) { return atan2f(y, x); };

    float gb_exp(float x)          { return expf(x); }
    float gb_log(float x)          { return logf(x); }
    float gb_pow(float x, float y) { return powf(x, y); }
#endif

float gb_exp2(float x) { return gb_exp(GB_MATH_LOG_TWO * x); }
float gb_log2(float x) { return gb_log(x) / GB_MATH_LOG_TWO; }


float gb_fast_exp(float x) {
    /* NOTE(bill): Only works in the range -1 <= x <= +1 */
    float e = 1.0f + x*(1.0f + x*0.5f*(1.0f + x*0.3333333333f*(1.0f + x*0.25f*(1.0f + x*0.2f))));
    return e;
}

float gb_fast_exp2(float x) { return gb_fast_exp(GB_MATH_LOG_TWO * x); }



float gb_round(float x) { return (float)((x >= 0.0f) ? gb_floor(x + 0.5f) : gb_ceil(x - 0.5f)); }
float gb_floor(float x) { return (float)((x >= 0.0f) ? (int)x : (int)(x-0.9999999999999999f)); }
float gb_ceil(float x)  { return (float)((x < 0) ? (int)x : ((int)x)+1); }





float gb_half_to_float(gbHalf value) {
    union { unsigned int i; float f; } result;
    int s = (value >> 15) & 0x001;
    int e = (value >> 10) & 0x01f;
    int m =  value        & 0x3ff;

    if (e == 0) {
        if (m == 0) {
            /* Plus or minus zero */
            result.i = (unsigned int)(s << 31);
            return result.f;
        } else {
            /* Denormalized number */
            while (!(m & 0x00000400)) {
                m <<= 1;
                e -=  1;
            }

            e += 1;
            m &= ~0x00000400;
        }
    } else if (e == 31) {
        if (m == 0) {
            /* Positive or negative infinity */
            result.i = (unsigned int)((s << 31) | 0x7f800000);
            return result.f;
        } else {
            /* Nan */
            result.i = (unsigned int)((s << 31) | 0x7f800000 | (m << 13));
            return result.f;
        }
    }

    e = e + (127 - 15);
    m = m << 13;

    result.i = (unsigned int)((s << 31) | (e << 23) | m);
    return result.f;
}

gbHalf gb_float_to_half(float value) {
    union { unsigned int i; float f; } v;
    int i, s, e, m;

    v.f = value;
    i = (int)v.i;

    s =  (i >> 16) & 0x00008000;
    e = ((i >> 23) & 0x000000ff) - (127 - 15);
    m =   i        & 0x007fffff;


    if (e <= 0) {
        if (e < -10) return (gbHalf)s;
        m = (m | 0x00800000) >> (1 - e);

        if (m & 0x00001000)
            m += 0x00002000;

        return (gbHalf)(s | (m >> 13));
    } else if (e == 0xff - (127 - 15)) {
        if (m == 0) {
            return (gbHalf)(s | 0x7c00); /* NOTE(bill): infinity */
        } else {
            /* NOTE(bill): NAN */
            m >>= 13;
            return (gbHalf)(s | 0x7c00 | m | (m == 0));
        }
    } else {
        if (m & 0x00001000) {
            m += 0x00002000;
            if (m & 0x00800000) {
                m = 0;
                e += 1;
            }
        }

        if (e > 30) {
            float volatile f = 1e12f;
            int j;
            for (j = 0; j < 10; j++)
                f *= f; /* NOTE(bill): Cause overflow */

            return (gbHalf)(s | 0x7c00);
        }

        return (gbHalf)(s | (e << 10) | (m >> 13));
    }
}
















#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
    gb_math_u64 gb_hash_murmur64(void const *key, size_t num_bytes, gb_math_u64 seed) {
        gb_math_u64 const m = 0xc6a4a7935bd1e995ULL;
        gb_math_u64 const r = 47;

        gb_math_u64 h = seed ^ (num_bytes * m);

        gb_math_u64 *data = (gb_math_u64 *)(key);
        gb_math_u64 *end = data + (num_bytes / 8);
        unsigned char *data2;

        while (data != end) {
            gb_math_u64 k = *data++;
            k *= m;
            k ^= k >> r;
            k *= m;
            h ^= k;
            h *= m;
        }

        data2 = (unsigned char *)data;

        switch (num_bytes & 7) {
        case 7: h ^= (gb_math_u64)data2[6] << 48;
        case 6: h ^= (gb_math_u64)data2[5] << 40;
        case 5: h ^= (gb_math_u64)data2[4] << 32;
        case 4: h ^= (gb_math_u64)data2[3] << 24;
        case 3: h ^= (gb_math_u64)data2[2] << 16;
        case 2: h ^= (gb_math_u64)data2[1] << 8;
        case 1: h ^= (gb_math_u64)data2[0];
            h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }
#else
    gb_math_u64 gb_hash_murmur64(void const *key, size_t num_bytes, gb_math_u64 seed) {
        gb_math_u32 const m = 0x5bd1e995;
        gb_math_u32 const r = 24;

        gb_math_u64 h  = 0;
        gb_math_u32 h1 = (gb_math_u32)seed ^ (gb_math_u32)num_bytes;
        gb_math_u32 h2 = (gb_math_u32)((gb_math_u64)seed >> 32);

        gb_math_u32 *data = (gb_math_u32 *)key;


        while (num_bytes >= 8) {
            gb_math_u32 k1, k2;
            k1 = *data++;
            k1 *= m;
            k1 ^= k1 >> r;
            k1 *= m;
            h1 *= m;
            h1 ^= k1;
            num_bytes -= 4;

            k2 = *data++;
            k2 *= m;
            k2 ^= k2 >> r;
            k2 *= m;
            h2 *= m;
            h2 ^= k2;
            num_bytes -= 4;
        }

        if (num_bytes >= 4) {
            gb_math_u32 k1 = *data++;
            k1 *= m;
            k1 ^= k1 >> r;
            k1 *= m;
            h1 *= m;
            h1 ^= k1;
            num_bytes -= 4;
        }

        switch (num_bytes) {
        gb_math_u32 a, b, c;
        case 3: c = data[2]; h2 ^= c << 16;
        case 2: b = data[1]; h2 ^= b <<  8;
        case 1: a = data[0]; h2 ^= a <<  0;
            h2 *= m;
        };

        h1 ^= h2 >> 18;
        h1 *= m;
        h2 ^= h1 >> 22;
        h2 *= m;
        h1 ^= h2 >> 17;
        h1 *= m;
        h2 ^= h1 >> 19;
        h2 *= m;

        h = (gb_math_u64)(h << 32) | (gb_math_u64)h2;

        return h;
    }
#endif


/* TODO(bill): Make better random number generators */
float gb_random_range_float(float min_inc, float max_inc) {
    int int_result = gb_random_range_int(0, 2147483646); /* Prevent integer overflow */
    float result = int_result/(float)2147483646;
    result *= max_inc - min_inc;
    result += min_inc;
    return result;
}

int gb_random_range_int(int min_inc, int max_inc) {
    static unsigned int random_value = 0xdeadbeef; /* Random Value */
    unsigned int diff, result;
    random_value = random_value * 2147001325 + 715136305; /* BCPL generator */
    diff = max_inc - min_inc + 1;
    result = random_value % diff;
    result += min_inc;

    return result;
}

float gb_random01(void) {
    return gb_random_range_float(0.0f, 1.0f);
}

#if defined(__GCC__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif



#endif /* GB_MATH_IMPLEMENTATION */