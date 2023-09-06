#pragma once
//#include "SDL\include\SDL_pixels.h"
#include "gb_math.h"
//#include "Misc.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <cstdint>

#define BIT(num) (1<<(num))
#define MATH_PREFIX [[nodiscard]] inline

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

union Color {
    struct { float r, g, b, a; };
    float e[4];
};

union ColorInt {
    u32 rgba;
    struct { u8 r, g, b, a; };
    u8 e[4];
};

MATH_PREFIX Color ToColor(ColorInt c)
{
    Color r;
    r.r = c.r / float(UCHAR_MAX);
    r.g = c.g / float(UCHAR_MAX);
    r.b = c.b / float(UCHAR_MAX);
    r.a = c.a / float(UCHAR_MAX);
    return r;
}

const Color Red         = { 1.00f, 0.00f, 0.00f, 1.00f };
const Color Green       = { 0.00f, 1.00f, 0.00f, 1.00f };
const Color Blue        = { 0.00f, 0.00f, 1.00f, 1.00f };
const Color transRed    = { 1.00f, 0.00f, 0.00f, 0.50f };
const Color transGreen  = { 0.00f, 1.00f, 0.00f, 0.50f };
const Color transBlue   = { 0.00f, 0.00f, 1.00f, 0.50f };
const Color transOrange = { 1.00f, 0.50f, 0.00f, 0.50f };
const Color transPurple = { 1.00f, 0.00f, 1.00f, 0.50f };
const Color lightRed    = { 1.00f, 0.00f, 0.00f, 0.25f };
const Color lightGreen  = { 0.00f, 1.00f, 0.00f, 0.25f };
const Color lightBlue   = { 0.00f, 0.00f, 1.00f, 0.25f };
const Color White       = { 1.00f, 1.00f, 1.00f, 1.00f };
const Color lightWhite  = { 0.58f, 0.58f, 0.58f, 0.58f };
const Color Black       = { 0.00f, 0.00f, 0.00f, 1.00f };
const Color lightBlack  = { 0.00f, 0.00f, 0.00f, 0.58f };
const Color Brown       = { 0.50f, 0.40f, 0.25f, 1.00f };
const Color Mint        = { 0.00f, 1.00f, 0.50f, 1.00f };
const Color Orange      = { 1.00f, 0.50f, 0.00f, 1.00f };
const Color Grey        = { 0.50f, 0.50f, 0.50f, 1.00f };
const Color transCyan   = { 0.60f, 0.60f, 1.00f, 0.50f };

const Color backgroundColor = { 0.263f, 0.706f, 0.965f, 0.0f };

const Color HealthBarBackground = { 0.25f, 0.33f, 0.25f, 1.0f};
constexpr i32 blockSize = 32;

const float pi = 3.14159f;
const float tau = 2 * pi;
const float inf = INFINITY;

typedef gbVec2 Vec2;
typedef gbVec3 Vec3;
typedef gbVec4 Vec4;
typedef gbMat2 Mat2;
typedef gbMat3 Mat3;
typedef gbMat4 Mat4;
typedef gbQuat Quat;

MATH_PREFIX Vec4 GetVec4(Vec3 a, float b)
{
    return { a.x, a.y, a.z, b };
}

union U32Pack {
    u32 pack;
    u32 rgba;
    u32 xyzw;
    struct { u8 r, g, b, a; };
    struct { u8 x, y, z, w; };
    u8 e[4];
};

struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
};

union Vec2Int {
    struct { i32 x, y; };
    i32 e[2];
};

union Vec3Int {
    struct { i32 x, y, z; };

    Vec2Int xy;
    i32 e[3];
};

union Vec2d {
    struct { double x, y; };
    double e[2];
};

struct RectInt {
    Vec2Int botLeft = {};
    Vec2Int topRight = {};

    i32 Width()
    {
        return topRight.x - botLeft.x;
    }

    i32 Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct Rect {
    Vec2 botLeft = {};
    Vec2 topRight = {};

    float Width()
    {
        return topRight.x - botLeft.x;
    }

    float Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct LineSegment {
    Vec2 p0;
    Vec2 p1;

    Vec2 Normal()
    {
        //TODO CHECK IF THIS IS RIGHT
        Vec2 result = (p1 - p0);
        result = { -result.y, result.x };
        return result;
    }
};


template <typename T = float>
struct Range {
    T min, max;

//    float RandomInRange()
//    {
//        return Random<float>(min, max);
//    }
//    void AngleSymetric(float angle, float range)
//    {
//        min = angle - range / 2;
//        max = angle + range / 2;
//    }
    //[[nodiscard]] inline T Center()
    MATH_PREFIX T Center() const
    {
        return min + ((max - min) / 2);
    }
};


//struct Rectangle {
//    Vec2 botLeft;
//    Vec2 topRight;
//
//    bool Collision(Vec2Int loc)
//    {
//        bool result = false;
//        if (loc.y > botLeft.x && loc.y < topRight.x)
//            if (loc.x > botLeft.x && loc.x < topRight.x)
//                result = true;
//        return result;
//    }
//};

struct Rectangle_Int {
    Vec2Int bottomLeft;
    Vec2Int topRight;

    i32 Width() const
    {
        return topRight.x - bottomLeft.x;
    }

    i32 Height() const
    {
        return topRight.y - bottomLeft.y;
    }
};

//inline Vec2 operator-(const Vec2& v)
//{
//    return { -v.x, -v.y };
//}
//
//inline Vec2 operator-(const Vec2& lhs, const float rhs)
//{
//    return { lhs.x - rhs, lhs.y - rhs };
//}


MATH_PREFIX Vec2Int operator*(const Vec2Int& a, const float b)
{
    return { int(a.x * b),  int(a.y * b) };
}

//inline Vec2 operator/(const Vec2& a, const float b)
//{
//    return { a.x / b,  a.y / b };
//}
//inline Vec2 operator/(const float lhs, const Vec2& rhs)
//{
//    return { lhs / rhs.x,  lhs / rhs.y };
//}
//
//inline bool operator==(const Rectangle& lhs, const Rectangle& rhs)
//{
//    bool bl = lhs.botLeft == rhs.botLeft;
//    bool tr = lhs.topRight == rhs.topRight;
//    return bl && tr;
//}
//
//inline bool operator!=(const Rectangle& lhs, const Rectangle& rhs)
//{
//    return !(lhs == rhs);
//}
MATH_PREFIX Vec3 operator+(Vec3 a, float b)
{
    Vec3 r = {a.x + b, a.y + b, a.z + b};
    return r;
}
MATH_PREFIX Vec3 operator-(Vec3 a, float b)
{
    Vec3 r = {a.x - b, a.y - b, a.z - b};
    return r;
}
inline void operator+=(Vec3& a, float b)
{
    a = {a.x + b, a.y + b, a.z + b};
}
inline void operator-=(Vec3& a, float b)
{
    a = {a.x - b, a.y - b, a.z - b};
}
MATH_PREFIX bool operator==(Vec3Int a, Vec3Int b)
{
    return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}
MATH_PREFIX bool operator>(Vec3Int a, Vec3Int b)
{
    return ((a.x > b.x) && (a.y > b.y) && (a.z > b.z));
}
MATH_PREFIX bool operator<(Vec3Int a, Vec3Int b)
{
    return ((a.x < b.x) && (a.y < b.y) && (a.z < b.z));
}
MATH_PREFIX bool operator>=(Vec3Int a, Vec3Int b)
{
    return ((a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z));
}
MATH_PREFIX bool operator<=(Vec3Int a, Vec3Int b)
{
    return ((a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z));
}
MATH_PREFIX Vec3Int operator-(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}
MATH_PREFIX Vec3Int operator+(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}
 inline Vec3Int& operator+=(Vec3Int &a, Vec3Int b)
{
    return (a = a + b);
}
inline Vec3Int& operator-=(Vec3Int &a, Vec3Int b)
{
    return (a = a - b);
}
//
MATH_PREFIX Vec3 operator/(float a, Vec3 b)
{
    Vec3 r = { a / b.x, a / b.y, a / b.z };
    return r;
}
MATH_PREFIX Vec2 operator/(float a, Vec2 b)
{
    Vec2 r = { a / b.x, a / b.y };
    return r;
}
MATH_PREFIX Vec3Int operator/(Vec3Int a, i32 b)
{
    Vec3Int r = {a.x / b, a.y / b, a.z / b };
    return r;
}
MATH_PREFIX Vec3Int operator/=(Vec3Int& a, i32 b)
{
    a = a / b;
}
MATH_PREFIX Vec2Int operator/(Vec2Int a, i32 b)
{
    Vec2Int r = {a.x / b, a.y / b };
    return r;
}
MATH_PREFIX Vec2Int operator/=(Vec2Int& a, i32 b)
{
    a = a / b;
}
//
MATH_PREFIX Vec3Int operator%(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x % b.x, a.y % b.y, a.z % b.z };
    return r;
}
MATH_PREFIX Vec3Int operator%=(Vec3Int& a, Vec3Int b)
{
    a = a % b;
}
//
MATH_PREFIX Vec3Int operator*(i32 a, Vec3Int b)
{
    Vec3Int r = { a * b.x, a * b.y, a * b.z };
    return r;
}
MATH_PREFIX Vec3Int operator*(Vec3Int a, i32 b)
{
    Vec3Int r = { a.x * b, a.y * b, a.z * b };
    return r;
}
MATH_PREFIX Vec2Int operator*(i32 a, Vec2Int b)
{
    Vec2Int r = { a * b.x, a * b.y };
    return r;
}
MATH_PREFIX Vec2Int operator*(Vec2Int a, i32 b)
{
    Vec2Int r = { a.x * b, a.y * b };
    return r;
}
//
MATH_PREFIX Vec3Int operator+(Vec3Int a, i32 b)
{
    Vec3Int r = { a.x + b, a.y + b, a.z + b };
    return r;
}
MATH_PREFIX Vec3Int operator+(i32 a, Vec3Int b)
{
    Vec3Int r = { a + b.x, a + b.y, a + b.z };
    return r;
}
MATH_PREFIX Vec3Int operator-(Vec3Int a, i32 b)
{
    Vec3Int r = { a.x - b, a.y - b, a.z - b };
    return r;
}
MATH_PREFIX Vec2Int operator+(Vec2Int a, i32 b)
{
    Vec2Int r = { a.x + b, a.y + b};
    return r;
}
MATH_PREFIX Vec2Int operator+(i32 a, Vec2Int b)
{
    Vec2Int r = { a + b.x, a + b.y };
    return r;
}
MATH_PREFIX Vec2Int operator-(Vec2Int a, i32 b)
{
    Vec2Int r = { a.x - b, a.y - b};
    return r;
}


MATH_PREFIX bool operator==(Vec2Int a, Vec2Int b)
{
    return ((a.x == b.x) && (a.y == b.y));
}
MATH_PREFIX bool operator>(Vec2Int a, Vec2Int b)
{
    return ((a.x > b.x) && (a.y > b.y));
}
MATH_PREFIX bool operator<(Vec2Int a, Vec2Int b)
{
    return ((a.x < b.x) && (a.y < b.y));
}
MATH_PREFIX bool operator>=(Vec2Int a, Vec2Int b)
{
    return ((a.x >= b.x) && (a.y >= b.y));
}
MATH_PREFIX bool operator<=(Vec2Int a, Vec2Int b)
{
    return ((a.x <= b.x) && (a.y <= b.y));
}
MATH_PREFIX Vec2Int operator-(Vec2Int a, Vec2Int b)
{
    Vec2Int r = {a.x - b.x, a.y - b.y };
    return r;
}
MATH_PREFIX Vec2Int operator+(Vec2Int a, Vec2Int b)
{
    Vec2Int r = {a.x + b.x, a.y + b.y };
    return r;
}
 inline Vec2Int &operator+=(Vec2Int &a, Vec2Int b)
{
    return (a = a + b);
}
inline Vec2Int &operator-=(Vec2Int &a, Vec2Int b)
{
    return (a = a - b);
}
MATH_PREFIX Vec2Int operator%(Vec2Int a, Vec2Int b)
{
    Vec2Int r = { a.x % b.x, a.y % b.y };
    return r;
}
MATH_PREFIX Vec2Int operator%=(Vec2Int& a, Vec2Int b)
{
    a = a % b;
}


MATH_PREFIX Vec3 operator+(float a, Vec3 b)
{
    Vec3 r = { a + b.x, a + b.y, a + b.z };
    return r;
}
MATH_PREFIX Vec3 operator-(float a, Vec3 b)
{
    Vec3 r = { a - b.x, a - b.y, a - b.z };
    return r;
}
MATH_PREFIX Vec2 operator+(Vec2 a, float b)
{
    Vec2 r = { a.x + b, a.y + b };
    return r;
}
MATH_PREFIX Vec2 operator+(float a, Vec2 b)
{
    Vec2 r = { a + b.x, a + b.y };
    return r;
}
MATH_PREFIX Vec2 operator-(Vec2 a, float b)
{
    Vec2 r = { a.x - b, a.y - b };
    return r;
}
MATH_PREFIX Vec2 operator-(float a, Vec2 b)
{
    Vec2 r = { a - b.x, a - b.y };
    return r;
}



template <typename T>
[[nodiscard]] T Min(const T a, const T b)
{
    return a < b ? a : b;
}

 template <typename T>
[[nodiscard]] T Max(const T a, const T b)
{
    return a > b ? a : b;
}

template <typename T>
[[nodiscard]] T Clamp(const T v, const T min, const T max)
{
    return Max(min, Min(max, v));
}

MATH_PREFIX Vec2 Floor(const Vec2& v)
{
    return { floorf(v.x), floorf(v.y) };
}

MATH_PREFIX Vec3 Floor(const Vec3& v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z) };
}

MATH_PREFIX Vec4 Floor(const Vec4& v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)  };
}

MATH_PREFIX Vec3 Trunc(const Vec3& v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z) };
}

MATH_PREFIX Vec4 Trunc(const Vec4& v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z), truncf(v.w)  };
}

MATH_PREFIX Vec3 Ceiling(const Vec3& v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z) };
}

MATH_PREFIX Vec4 Ceiling(const Vec4& v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z), ceilf(v.w)  };
}

MATH_PREFIX float Fract(float a)
{
    return a - floorf(a);
}

MATH_PREFIX Vec2 Fract(const Vec2& a)
{
    return a - Floor(a);
}

MATH_PREFIX Vec3 Fract(const Vec3& a)
{
    return a - Floor(a);
}

MATH_PREFIX Vec4 Fract(const Vec4& a)
{
    return a - Floor(a);
}

MATH_PREFIX float Abs(float a)
{
    return fabs(a);
}
MATH_PREFIX Vec3 Abs(const Vec3& a)
{
    return { fabs(a.x), fabs(a.y), fabs(a.z) };
}
MATH_PREFIX Vec3 Abs(const Vec2& a)
{
    return { abs(a.x), abs(a.y) };
}
MATH_PREFIX Vec3Int Abs(const Vec3Int& a)
{
    return { abs(a.x), abs(a.y), abs(a.z) };
}
MATH_PREFIX Vec2Int Abs(const Vec2Int& a)
{
    return { abs(a.x), abs(a.y) };
}

MATH_PREFIX Vec2 Sine(const Vec2& v)
{
    Vec2 r = { sinf(v.x), sinf(v.y) };
    return r;
}

MATH_PREFIX Vec3 Sine(const Vec3& v)
{
    Vec3 r = { sinf(v.x), sinf(v.y), sinf(v.z) };
    return r;
}

//MATH AND CONVERSIONS

MATH_PREFIX float RadToDeg(float angle)
{
    return ((angle) / (tau)) * 360;
}

MATH_PREFIX float DegToRad(float angle)
{
    return (angle / 360 ) * (tau);
}

//TODO: Improve
MATH_PREFIX Vec3 NormalizeZero(const Vec3& v)
{
    float prod = v.x * v.x + v.y * v.y + v.z * v.z;
    if (prod == 0.0f)
        return {};
    float hyp = sqrtf(prod);
    Vec3 result = { (v.x / hyp), (v.y / hyp), (v.z / hyp)};
    return result;
}
MATH_PREFIX Vec2 NormalizeZero(const Vec2& v)
{
    float prod = v.x * v.x + v.y * v.y;
    if (prod == 0.0f)
        return {};
    float hyp = sqrtf(prod);
    Vec2 result = { (v.x / hyp), (v.y / hyp) };
    return result;
}
MATH_PREFIX Vec3 Normalize(const Vec3& v)
{
    float hyp = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    Vec3 result = { (v.x / hyp), (v.y / hyp) , (v.z / hyp)};
    return result;
}
MATH_PREFIX Vec2 Normalize(const Vec2& v)
{
    float hyp = sqrtf((v.x * v.x) + (v.y * v.y));
    return { (v.x / hyp), (v.y / hyp) };
}
inline float* Normalize(float* v, size_t length)
{
    float total = 0;
    for (i32 i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    float hyp = sqrtf(total);
    for (i32 i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}
inline double* Normalize(double* v, size_t length)
{
    double total = 0;
    for (i32 i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    double hyp = sqrt(total);
    for (i32 i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}

template <typename T>
MATH_PREFIX T Lerp(const T& a, const T& b, float t)
{
    return a + (b - a) * t;
}

MATH_PREFIX Vec3 Converge(const Vec3& value, const Vec3& target, float rate, float dt)
{
    return Lerp(target, value, exp2(-rate * dt));
}

#if 1
float Bilinear(float p00, float p10, float p01, float p11, float x, float y);
#else
[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr);
#endif
[[nodiscard]] float Cubic(Vec4 v, float x);
[[nodiscard]] float Bicubic(Mat4 p, Vec2 pos);

/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        pi/4
*/

MATH_PREFIX float DotProduct(const Vec2& a, const Vec2& b)
{
    float r = a.x * b.x + a.y * b.y;
    return r;
}
MATH_PREFIX float DotProduct(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
MATH_PREFIX Vec3 CrossProduct(const Vec3& a, const Vec3& b)
{
    return gb_vec3_cross(a, b);
}
MATH_PREFIX float CrossProduct(const Vec2& a, const Vec2& b)
{
    return gb_vec2_cross(a, b);
}

MATH_PREFIX float Pythags(const Vec2& a)
{
    return sqrtf((a.x * a.x) + (a.y * a.y));
}
MATH_PREFIX float Pythags(const Vec3& a)
{
    return sqrtf((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
}
MATH_PREFIX float Pythags(const Vec2Int& a)
{
    return Pythags(Vec2({ float(a.x), float(a.y) }));
}
MATH_PREFIX float Pythags(const Vec3Int& a)
{
    return Pythags(Vec3({ float(a.x), float(a.y), float(a.z) }));
}

MATH_PREFIX float Distance(const Vec2& a, const Vec2& b)
{
    return Pythags(a - b);
}
MATH_PREFIX double DistanceD(const Vec2& a, const Vec2& b)
{
    return Pythags(a - b);
}
MATH_PREFIX double Distance(const Vec3Int& a, const Vec3Int& b)
{
    return Pythags(a - b);
}
MATH_PREFIX double Distance(const Vec2Int& a, const Vec2Int& b)
{
    return Pythags(a - b);
}
MATH_PREFIX float Distance(const Vec3& a, const Vec3& b)
{
    return Pythags(a - b);
}

MATH_PREFIX float Length(const Vec2& a)
{
    return Pythags(a);
}
MATH_PREFIX double LengthD(const Vec2& a)
{
    return Pythags(a);
}
MATH_PREFIX double Length(const Vec3Int& a)
{
    return Pythags(a);
}
MATH_PREFIX double Length(const Vec2Int& a)
{
    return Pythags(a);
}
MATH_PREFIX float Length(const Vec3& a)
{
    return Pythags(a);
}
MATH_PREFIX Vec3 Acos(const Vec3& a)
{
    return { acos(a.x), acos(a.y), acos(a.z) };
}

MATH_PREFIX Vec3 Round(const Vec2& a)
{
    Vec3 r = { roundf(a.x), roundf(a.y) };
    return r;
}
MATH_PREFIX Vec3 Round(const Vec3& a)
{
    Vec3 r = { roundf(a.x), roundf(a.y), roundf(a.z) };
    return r;
}
MATH_PREFIX Vec4 Round(const Vec4& a)
{
    Vec4 r = { roundf(a.x), roundf(a.y), roundf(a.z), roundf(a.w) };
    return r;
}
MATH_PREFIX float Sign(float value)
{
    return value < 0.0f ? -1.0f : 1.0f;
}
MATH_PREFIX i32 Sign(i32 value)
{
    return value < 0 ? -1 : 1;
}


[[nodiscard]] u32 PCG_Random(u64 state);
MATH_PREFIX u32 RandomU32(u64 state, u32 min, u32 max)
{
    u32 result = PCG_Random(state);
    //uint32 result = state;
    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
    return (result % (max - min)) + min;
}

MATH_PREFIX float RandomFloat(const float min, const float max)
{
    return min + (max - min) * (rand() / float(RAND_MAX));
}

MATH_PREFIX Vec3Int ToVec3Int(const Vec3& a)
{
    return { static_cast<i32>(a.x), static_cast<i32>(a.y), static_cast<i32>(a.z) };
}
MATH_PREFIX Vec3    ToVec3(const Vec3Int& a)
{
    return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) };
}
MATH_PREFIX Vec2Int ToVec2Int(const Vec2& a)
{
    return { static_cast<i32>(a.x), static_cast<i32>(a.y) };
}
MATH_PREFIX Vec2    ToVec2(const Vec2Int& a)
{
    return { static_cast<float>(a.x), static_cast<float>(a.y) };
}

//Multiplication of two vectors without adding each dimension to get the dot product
MATH_PREFIX Vec3Int HadamardProduct(const Vec3Int& a, const Vec3Int& b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}
//Multiplication of two vectors without adding each dimension to get the dot product
MATH_PREFIX Vec3 HadamardProduct(const Vec3& a, const Vec3& b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}

struct Plane
{
   float x,y,z,w;
};

struct Frustum {
    Plane e[6];
};

struct AABB {
    Vec3 min = {};
    Vec3 max = {};

    [[nodiscard]] Vec3 GetLengths() const
    {
        Vec3 result = {};
        result.x = Abs(max.x - min.x);
        result.y = Abs(max.y - min.y);
        result.z = Abs(max.z - min.z);
        return result;
    }

    [[nodiscard]] Vec3 Center() const
    {
        Vec3 result = {};
        result = min + ((max - min) / 2);
        return result;
    }
};


struct VertexFace {

    Vec3 e[4];
};

//static const Vec3 cubeVertices[] = {
//    // +x
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    // -x
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    // +y
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    // -y
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    // z
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    // -z
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//};

const VertexFace cubeVertices[6] = {
    // +x
    VertexFace( {
    Vec3(1.0,  1.0,  1.0),
    Vec3(1.0,  0.0,  1.0),
    Vec3(1.0,  1.0,  0.0),
    Vec3(1.0,  0.0,  0.0)
    }),
    // -x
    VertexFace({
    Vec3(0.0,  1.0,  0.0),
    Vec3(0.0,  0.0,  0.0),
    Vec3(0.0,  1.0,  1.0),
    Vec3(0.0,  0.0,  1.0)
    }),
    // +y
    VertexFace({
    Vec3(1.0,  1.0,  1.0 ),
    Vec3(1.0,  1.0,  0.0 ),
    Vec3(0.0,  1.0,  1.0 ),
    Vec3(0.0,  1.0,  0.0 )
    }),
    // -y
    VertexFace({
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(0.0,  0.0,  0.0 ),
    Vec3(1.0,  0.0,  1.0 ),
    Vec3(1.0,  0.0,  0.0 )
    }),
    // z
    VertexFace({
    Vec3(0.0,  1.0,  1.0 ),
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(1.0,  1.0,  1.0 ),
    Vec3(1.0,  0.0,  1.0 )
    }),
    // -z
    VertexFace({
    Vec3(1.0,  1.0,  0.0 ),
    Vec3(1.0,  0.0,  0.0 ),
    Vec3(0.0,  1.0,  0.0 ),
    Vec3(0.0,  0.0,  0.0 )
    })
};

const VertexFace smallCubeVertices[6] = {
    // +x
    VertexFace( {
    Vec3(1.0,  0.5,  1.0),
    Vec3(1.0,  0.0,  1.0),
    Vec3(1.0,  0.5,  0.0),
    Vec3(1.0,  0.0,  0.0)
    }),
    // -x
    VertexFace({
    Vec3(0.0,  0.5,  0.0),
    Vec3(0.0,  0.0,  0.0),
    Vec3(0.0,  0.5,  1.0),
    Vec3(0.0,  0.0,  1.0)
    }),
    // +y
    VertexFace({
    Vec3(1.0,  0.5,  1.0 ),
    Vec3(1.0,  0.5,  0.0 ),
    Vec3(0.0,  0.5,  1.0 ),
    Vec3(0.0,  0.5,  0.0 )
    }),
    // -y
    VertexFace({
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(0.0,  0.0,  0.0 ),
    Vec3(1.0,  0.0,  1.0 ),
    Vec3(1.0,  0.0,  0.0 )
    }),
    // z
    VertexFace({
    Vec3(0.0,  0.5,  1.0 ),
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(1.0,  0.5,  1.0 ),
    Vec3(1.0,  0.0,  1.0 )
    }),
    // -z
    VertexFace({
    Vec3(1.0,  0.5,  0.0 ),
    Vec3(1.0,  0.0,  0.0 ),
    Vec3(0.0,  0.5,  0.0 ),
    Vec3(0.0,  0.0,  0.0 )
    })
};


union Triangle {
    struct { Vec3 p0, p1, p2; };
    Vec3 e[3];

    Vec3 Normal() const
    {
        Vec3 r = Normalize(CrossProduct(p1 - p0, p2 - p0));
        return r;
    }
    Vec3 Center() const
    {
        Vec3 r = (p0 + p1 + p2) / 3;
        return r;
    }
};

Frustum ComputeFrustum(const Mat4& mvProj);
bool IsBoxInFrustum(const Frustum& f, float* bmin, float* bmax);
i32 ManhattanDistance(Vec3Int a, Vec3Int b);


//Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point);

//void QuickSort(uint8* data, const int32 length, const int32 itemSize, int32 (*compare)(const void* a, const void* b));
