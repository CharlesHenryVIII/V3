#pragma once


union Vec2_256 {
    struct { __m256 x, y; };

    __m256 e[2];
};
union Vec3_256 {
    struct { __m256 x, y, z; };
    struct { __m256 r, g, b; };

    Vec2_256 xy;
    __m256 e[3];
};

#define SIMD_PREFIX [[nodiscard]] inline

SIMD_PREFIX __m256 DotProduct_256(const Vec3_256 a, const Vec3_256 b)
{
    __m256 _x = _mm256_mul_ps(a.x, b.x);
    __m256 _y = _mm256_mul_ps(a.y, b.y);
    __m256 _z = _mm256_mul_ps(a.z, b.z);

    __m256 r = _mm256_add_ps(_x, _y);
           r = _mm256_add_ps( r, _z);
    return r;
}
SIMD_PREFIX __m256 DotProduct_256(const __m256 a_x,
                                  const __m256 a_y,
                                  const __m256 a_z,
                                  const __m256 b_x,
                                  const __m256 b_y,
                                  const __m256 b_z)
{
    __m256 _x = _mm256_mul_ps(a_x, b_x);
    __m256 _y = _mm256_mul_ps(a_y, b_y);
    __m256 _z = _mm256_mul_ps(a_z, b_z);

    __m256 r = _mm256_add_ps(_x, _y);
           r = _mm256_add_ps( r, _z);
    return r;
}


SIMD_PREFIX __m256 LengthSquared_256(const Vec3_256 a)
{
    __m256 _x = _mm256_mul_ps(a.x, a.x);
    __m256 _y = _mm256_mul_ps(a.y, a.y);
    __m256 _z = _mm256_mul_ps(a.z, a.z);

    __m256 r = _mm256_add_ps(_x, _y);
    r = _mm256_add_ps(r, _z);
    return r;
}
SIMD_PREFIX __m256 LengthSquared_256(
                const __m256 x,
                const __m256 y,
                const __m256 z)
{
    __m256 _x = _mm256_mul_ps(x, x);
    __m256 _y = _mm256_mul_ps(y, y);
    __m256 _z = _mm256_mul_ps(z, z);

    __m256 r = _mm256_add_ps(_x, _y);
    r = _mm256_add_ps(r, _z);
    return r;
}


SIMD_PREFIX __m256 Length_256(const Vec3_256 a)
{
    __m256 inner = LengthSquared_256(a);
    __m256 r = _mm256_sqrt_ps(inner);
    return r;
}
SIMD_PREFIX __m256 Length_256(
                const __m256 x,
                const __m256 y,
                const __m256 z)
{
    __m256 inner = LengthSquared_256(x, y, z);
    __m256 r = _mm256_sqrt_ps(inner);
    return r;
}

SIMD_PREFIX Vec3_256 Normalize_256(const Vec3_256 b)
{
    __m256 py = LengthSquared_256(b);
    py = _mm256_rsqrt_ps(py);
    Vec3_256 r;
    r.x = _mm256_mul_ps(b.x, py);
    r.y = _mm256_mul_ps(b.y, py);
    r.z = _mm256_mul_ps(b.z, py);
    return r;
}
SIMD_PREFIX void Normalize_256(
                __m256& out_x,
                __m256& out_y,
                __m256& out_z,
         const  __m256 x,
         const  __m256 y,
         const  __m256 z)
{
    __m256 py = LengthSquared_256(x, y, z);
    py = _mm256_rsqrt_ps(py);
    out_x = _mm256_mul_ps(x, py);
    out_y = _mm256_mul_ps(y, py);
    out_z = _mm256_mul_ps(z, py);
}

SIMD_PREFIX Vec3_256 And_256(const Vec3_256 a, const __m256 b)
{
    Vec3_256 r;
    r.x = _mm256_and_ps(a.x, b);
    r.y = _mm256_and_ps(a.y, b);
    r.z = _mm256_and_ps(a.z, b);
    return r;
}
SIMD_PREFIX Vec3_256 Add_256(const Vec3_256 a, const Vec3_256 b)
{
    Vec3_256 r;
    r.x = _mm256_add_ps(a.x, b.x);
    r.y = _mm256_add_ps(a.y, b.y);
    r.z = _mm256_add_ps(a.z, b.z);
    return r;
}
SIMD_PREFIX Vec3_256 Subtract_256(const Vec3_256 a, const Vec3_256 b)
{
    Vec3_256 r;
    r.x = _mm256_sub_ps(a.x, b.x);
    r.y = _mm256_sub_ps(a.y, b.y);
    r.z = _mm256_sub_ps(a.z, b.z);
    return r;
}
SIMD_PREFIX Vec3_256 Multiply_256(const Vec3_256 a, const __m256 b)
{
    Vec3_256 r;
    r.x = _mm256_mul_ps(a.x, b);
    r.y = _mm256_mul_ps(a.y, b);
    r.z = _mm256_mul_ps(a.z, b);
    return r;
}

SIMD_PREFIX Vec3_256 Divide_256(const Vec3_256 a, const __m256 b)
{
    Vec3_256 r;
    r.x = _mm256_div_ps(a.x, b);
    r.y = _mm256_div_ps(a.y, b);
    r.z = _mm256_div_ps(a.z, b);
    return r;
}

#if 0
[[nodiscard]] inline Vec3_256 operator-(Vec3_256 a, Vec3_256 b) { return Subtract(a, b); }
#endif
