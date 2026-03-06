#include <immintrin.h>
#include <vector>
#include <cmath>

struct Vec3 { float x, y, z; };

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

struct TrianglesSoA
{
    bool valid = false;
    std::vector<float> v0x, v0y, v0z;
    std::vector<float> v1x, v1y, v1z;
    std::vector<float> v2x, v2y, v2z;

    void set(float* vertices, size_t size) {
        if (size % 9 != 0) return;

        size_t triangleCount = size / 9;

        v0x.resize(triangleCount);
        v0y.resize(triangleCount);
        v0z.resize(triangleCount);

        v1x.resize(triangleCount);
        v1y.resize(triangleCount);
        v1z.resize(triangleCount);

        v2x.resize(triangleCount);
        v2y.resize(triangleCount);
        v2z.resize(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            size_t base = i * 9;

            v0x[i] = vertices[base + 0];
            v0y[i] = vertices[base + 1];
            v0z[i] = vertices[base + 2];

            v1x[i] = vertices[base + 3];
            v1y[i] = vertices[base + 4];
            v1z[i] = vertices[base + 5];

            v2x[i] = vertices[base + 6];
            v2y[i] = vertices[base + 7];
            v2z[i] = vertices[base + 8];
        }

        valid = true;
    }

    size_t size() const { return v0x.size(); }
};

bool anyHitSIMD(
    const Vec3& rayStart,
    const Vec3& rayEnd,
    const TrianglesSoA& tris)
{
    const size_t n = tris.size();
    if (n == 0)
        return false;

    Vec3 dir = {
        rayEnd.x - rayStart.x,
        rayEnd.y - rayStart.y,
        rayEnd.z - rayStart.z
    };

    const float EPS = 1e-8f;
    const float RAY_EPS = 1e-6f;

    const __m256 dirx = _mm256_set1_ps(dir.x);
    const __m256 diry = _mm256_set1_ps(dir.y);
    const __m256 dirz = _mm256_set1_ps(dir.z);

    const __m256 rayStartX = _mm256_set1_ps(rayStart.x);
    const __m256 rayStartY = _mm256_set1_ps(rayStart.y);
    const __m256 rayStartZ = _mm256_set1_ps(rayStart.z);

    const __m256 zero = _mm256_set1_ps(0.0f);
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 eps = _mm256_set1_ps(EPS);
    const __m256 rayMin = _mm256_set1_ps(-RAY_EPS);
    const __m256 rayMax = _mm256_set1_ps(1.0f + RAY_EPS);

    size_t i = 0;
    const size_t simdEnd = (n / 8) * 8;

    for (; i < simdEnd; i += 8)
    {
        __m256 v0x = _mm256_loadu_ps(&tris.v0x[i]);
        __m256 v0y = _mm256_loadu_ps(&tris.v0y[i]);
        __m256 v0z = _mm256_loadu_ps(&tris.v0z[i]);

        __m256 v1x = _mm256_loadu_ps(&tris.v1x[i]);
        __m256 v1y = _mm256_loadu_ps(&tris.v1y[i]);
        __m256 v1z = _mm256_loadu_ps(&tris.v1z[i]);

        __m256 v2x = _mm256_loadu_ps(&tris.v2x[i]);
        __m256 v2y = _mm256_loadu_ps(&tris.v2y[i]);
        __m256 v2z = _mm256_loadu_ps(&tris.v2z[i]);

        __m256 e1x = _mm256_sub_ps(v1x, v0x);
        __m256 e1y = _mm256_sub_ps(v1y, v0y);
        __m256 e1z = _mm256_sub_ps(v1z, v0z);

        __m256 e2x = _mm256_sub_ps(v2x, v0x);
        __m256 e2y = _mm256_sub_ps(v2y, v0y);
        __m256 e2z = _mm256_sub_ps(v2z, v0z);

        __m256 hx = _mm256_sub_ps(_mm256_mul_ps(diry, e2z), _mm256_mul_ps(dirz, e2y));
        __m256 hy = _mm256_sub_ps(_mm256_mul_ps(dirz, e2x), _mm256_mul_ps(dirx, e2z));
        __m256 hz = _mm256_sub_ps(_mm256_mul_ps(dirx, e2y), _mm256_mul_ps(diry, e2x));

        __m256 a =
            _mm256_add_ps(
                _mm256_add_ps(_mm256_mul_ps(e1x, hx), _mm256_mul_ps(e1y, hy)),
                _mm256_mul_ps(e1z, hz));

        __m256 absA = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), a);

        // not parallel mask
        __m256 maskA = _mm256_cmp_ps(absA, eps, _CMP_GE_OS);

        __m256 f = _mm256_div_ps(one, a);

        __m256 sx = _mm256_sub_ps(rayStartX, v0x);
        __m256 sy = _mm256_sub_ps(rayStartY, v0y);
        __m256 sz = _mm256_sub_ps(rayStartZ, v0z);

        __m256 u =
            _mm256_mul_ps(f,
                _mm256_add_ps(
                    _mm256_add_ps(_mm256_mul_ps(sx, hx), _mm256_mul_ps(sy, hy)),
                    _mm256_mul_ps(sz, hz)));

        __m256 maskU =
            _mm256_and_ps(
                _mm256_cmp_ps(u, zero, _CMP_GE_OS),
                _mm256_cmp_ps(u, one, _CMP_LE_OS));

        __m256 qx = _mm256_sub_ps(_mm256_mul_ps(sy, e1z), _mm256_mul_ps(sz, e1y));
        __m256 qy = _mm256_sub_ps(_mm256_mul_ps(sz, e1x), _mm256_mul_ps(sx, e1z));
        __m256 qz = _mm256_sub_ps(_mm256_mul_ps(sx, e1y), _mm256_mul_ps(sy, e1x));

        __m256 v =
            _mm256_mul_ps(f,
                _mm256_add_ps(
                    _mm256_add_ps(_mm256_mul_ps(dirx, qx), _mm256_mul_ps(diry, qy)),
                    _mm256_mul_ps(dirz, qz)));

        __m256 maskV =
            _mm256_and_ps(
                _mm256_cmp_ps(v, zero, _CMP_GE_OS),
                _mm256_cmp_ps(_mm256_add_ps(u, v), one, _CMP_LE_OS));

        __m256 t =
            _mm256_mul_ps(f,
                _mm256_add_ps(
                    _mm256_add_ps(_mm256_mul_ps(e2x, qx), _mm256_mul_ps(e2y, qy)),
                    _mm256_mul_ps(e2z, qz)));

        __m256 maskT =
            _mm256_and_ps(
                _mm256_cmp_ps(t, rayMin, _CMP_GE_OS),
                _mm256_cmp_ps(t, rayMax, _CMP_LE_OS));

        __m256 finalMask =
            _mm256_and_ps(
                _mm256_and_ps(maskA, maskU),
                _mm256_and_ps(maskV, maskT));

        if (_mm256_movemask_ps(finalMask) != 0)
            return true;
    }

    // Scalar tail
    for (; i < n; ++i)
    {
        Vec3 e1 = { tris.v1x[i] - tris.v0x[i],
                    tris.v1y[i] - tris.v0y[i],
                    tris.v1z[i] - tris.v0z[i] };

        Vec3 e2 = { tris.v2x[i] - tris.v0x[i],
                    tris.v2y[i] - tris.v0y[i],
                    tris.v2z[i] - tris.v0z[i] };

        Vec3 h = cross(dir, e2);
        float a = dot(e1, h);

        if (std::fabs(a) < EPS)
            continue;

        float f = 1.0f / a;

        Vec3 s = { rayStart.x - tris.v0x[i],
                   rayStart.y - tris.v0y[i],
                   rayStart.z - tris.v0z[i] };

        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f)
            continue;

        Vec3 q = cross(s, e1);
        float vVal = f * dot(dir, q);
        if (vVal < 0.0f || u + vVal > 1.0f)
            continue;

        float tVal = f * dot(e2, q);
        if (tVal >= -RAY_EPS && tVal <= 1.0f + RAY_EPS)
            return true;
    }

    return false;
}

bool anyHitAVX512(
    const Vec3& rayStart,
    const Vec3& rayEnd,
    const TrianglesSoA& tris)
{
    const size_t n = tris.size();
    if (n == 0)
        return false;

    Vec3 dir = {
        rayEnd.x - rayStart.x,
        rayEnd.y - rayStart.y,
        rayEnd.z - rayStart.z
    };

    const float EPS = 1e-8f;
    const float RAY_EPS = 1e-6f;

    const __m512 dirx = _mm512_set1_ps(dir.x);
    const __m512 diry = _mm512_set1_ps(dir.y);
    const __m512 dirz = _mm512_set1_ps(dir.z);

    const __m512 rayStartX = _mm512_set1_ps(rayStart.x);
    const __m512 rayStartY = _mm512_set1_ps(rayStart.y);
    const __m512 rayStartZ = _mm512_set1_ps(rayStart.z);

    const __m512 zero = _mm512_set1_ps(0.0f);
    const __m512 one = _mm512_set1_ps(1.0f);
    const __m512 eps = _mm512_set1_ps(EPS);
    const __m512 rayMin = _mm512_set1_ps(-RAY_EPS);
    const __m512 rayMax = _mm512_set1_ps(1.0f + RAY_EPS);

    size_t i = 0;
    const size_t simdEnd = (n / 16) * 16;

    for (; i < simdEnd; i += 16)
    {
        __m512 v0x = _mm512_loadu_ps(&tris.v0x[i]);
        __m512 v0y = _mm512_loadu_ps(&tris.v0y[i]);
        __m512 v0z = _mm512_loadu_ps(&tris.v0z[i]);

        __m512 v1x = _mm512_loadu_ps(&tris.v1x[i]);
        __m512 v1y = _mm512_loadu_ps(&tris.v1y[i]);
        __m512 v1z = _mm512_loadu_ps(&tris.v1z[i]);

        __m512 v2x = _mm512_loadu_ps(&tris.v2x[i]);
        __m512 v2y = _mm512_loadu_ps(&tris.v2y[i]);
        __m512 v2z = _mm512_loadu_ps(&tris.v2z[i]);

        __m512 e1x = _mm512_sub_ps(v1x, v0x);
        __m512 e1y = _mm512_sub_ps(v1y, v0y);
        __m512 e1z = _mm512_sub_ps(v1z, v0z);

        __m512 e2x = _mm512_sub_ps(v2x, v0x);
        __m512 e2y = _mm512_sub_ps(v2y, v0y);
        __m512 e2z = _mm512_sub_ps(v2z, v0z);

        __m512 hx = _mm512_sub_ps(_mm512_mul_ps(diry, e2z), _mm512_mul_ps(dirz, e2y));
        __m512 hy = _mm512_sub_ps(_mm512_mul_ps(dirz, e2x), _mm512_mul_ps(dirx, e2z));
        __m512 hz = _mm512_sub_ps(_mm512_mul_ps(dirx, e2y), _mm512_mul_ps(diry, e2x));

        __m512 a =
            _mm512_add_ps(
                _mm512_add_ps(_mm512_mul_ps(e1x, hx), _mm512_mul_ps(e1y, hy)),
                _mm512_mul_ps(e1z, hz));

        __m512 absA = _mm512_abs_ps(a);

        __mmask16 maskA = _mm512_cmp_ps_mask(absA, eps, _CMP_GE_OS);

        __m512 f = _mm512_div_ps(one, a);

        __m512 sx = _mm512_sub_ps(rayStartX, v0x);
        __m512 sy = _mm512_sub_ps(rayStartY, v0y);
        __m512 sz = _mm512_sub_ps(rayStartZ, v0z);

        __m512 u =
            _mm512_mul_ps(f,
                _mm512_add_ps(
                    _mm512_add_ps(_mm512_mul_ps(sx, hx), _mm512_mul_ps(sy, hy)),
                    _mm512_mul_ps(sz, hz)));

        __mmask16 maskU =
            _mm512_cmp_ps_mask(u, zero, _CMP_GE_OS) &
            _mm512_cmp_ps_mask(u, one, _CMP_LE_OS);

        __m512 qx = _mm512_sub_ps(_mm512_mul_ps(sy, e1z), _mm512_mul_ps(sz, e1y));
        __m512 qy = _mm512_sub_ps(_mm512_mul_ps(sz, e1x), _mm512_mul_ps(sx, e1z));
        __m512 qz = _mm512_sub_ps(_mm512_mul_ps(sx, e1y), _mm512_mul_ps(sy, e1x));

        __m512 v =
            _mm512_mul_ps(f,
                _mm512_add_ps(
                    _mm512_add_ps(_mm512_mul_ps(dirx, qx), _mm512_mul_ps(diry, qy)),
                    _mm512_mul_ps(dirz, qz)));

        __mmask16 maskV =
            _mm512_cmp_ps_mask(v, zero, _CMP_GE_OS) &
            _mm512_cmp_ps_mask(_mm512_add_ps(u, v), one, _CMP_LE_OS);

        __m512 t =
            _mm512_mul_ps(f,
                _mm512_add_ps(
                    _mm512_add_ps(_mm512_mul_ps(e2x, qx), _mm512_mul_ps(e2y, qy)),
                    _mm512_mul_ps(e2z, qz)));

        __mmask16 maskT =
            _mm512_cmp_ps_mask(t, rayMin, _CMP_GE_OS) &
            _mm512_cmp_ps_mask(t, rayMax, _CMP_LE_OS);

        __mmask16 finalMask = maskA & maskU & maskV & maskT;

        if (finalMask)
            return true;
    }

    for (; i < n; ++i) {
        Vec3 e1 = { tris.v1x[i] - tris.v0x[i],
                    tris.v1y[i] - tris.v0y[i],
                    tris.v1z[i] - tris.v0z[i] };

        Vec3 e2 = { tris.v2x[i] - tris.v0x[i],
                    tris.v2y[i] - tris.v0y[i],
                    tris.v2z[i] - tris.v0z[i] };

        Vec3 h = cross(dir, e2);
        float a = dot(e1, h);

        if (std::fabs(a) < EPS)
            continue;

        float f = 1.0f / a;

        Vec3 s = { rayStart.x - tris.v0x[i],
                   rayStart.y - tris.v0y[i],
                   rayStart.z - tris.v0z[i] };

        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f)
            continue;

        Vec3 q = cross(s, e1);
        float vVal = f * dot(dir, q);
        if (vVal < 0.0f || u + vVal > 1.0f)
            continue;

        float tVal = f * dot(e2, q);
        if (tVal >= -RAY_EPS && tVal <= 1.0f + RAY_EPS)
            return true;
    }

    return false;
}

bool cpuSupportsAVX512() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    int nIds = cpuInfo[0];

    if (nIds >= 7) {
        __cpuidex(cpuInfo, 7, 0);
        return (cpuInfo[1] & (1 << 16)) != 0;
    }
    return false;
}