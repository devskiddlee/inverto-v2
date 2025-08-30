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

bool anyHitSIMD(const Vec3& rayStart, const Vec3& rayEnd,
                const std::vector<Vec3>& v0,
                const std::vector<Vec3>& v1,
                const std::vector<Vec3>& v2)
{
    size_t n = v0.size();
    Vec3 dir = {rayEnd.x - rayStart.x, rayEnd.y - rayStart.y, rayEnd.z - rayStart.z};
    const float EPS = 1e-8f;

    for (size_t i = 0; i < n; i += 4) {
        bool hit[4] = {false,false,false,false};

        for (int j = 0; j < 4 && (i+j) < n; ++j) {
            Vec3 e1 = {v1[i+j].x - v0[i+j].x, v1[i+j].y - v0[i+j].y, v1[i+j].z - v0[i+j].z};
            Vec3 e2 = {v2[i+j].x - v0[i+j].x, v2[i+j].y - v0[i+j].y, v2[i+j].z - v0[i+j].z};

            Vec3 h = cross(dir, e2);
            float a = dot(e1, h);
            if (std::fabs(a) < EPS) continue;

            float f = 1.0f / a;
            Vec3 s = {rayStart.x - v0[i+j].x, rayStart.y - v0[i+j].y, rayStart.z - v0[i+j].z};
            float u = f * dot(s, h);
            if (u < 0.0f || u > 1.0f) continue;

            Vec3 q = cross(s, e1);
            float v = f * dot(dir, q);
            if (v < 0.0f || u + v > 1.0f) continue;

            float t = f * dot(e2, q);
            if (t >= 0.0f && t <= 1.0f) return true;
        }
    }

    return false;
}

bool anyHitAVX512(const Vec3& rayStart, const Vec3& rayEnd,
    const std::vector<Vec3>& v0,
    const std::vector<Vec3>& v1,
    const std::vector<Vec3>& v2)
{
    size_t n = v0.size();
    Vec3 dir = { rayEnd.x - rayStart.x, rayEnd.y - rayStart.y, rayEnd.z - rayStart.z };
    const float EPS = 1e-8f;

    size_t i = 0;
    size_t simdEnd = n / 8 * 8;

    __m512 dirx = _mm512_set1_ps(dir.x);
    __m512 diry = _mm512_set1_ps(dir.y);
    __m512 dirz = _mm512_set1_ps(dir.z);

    __m512 rayStartX = _mm512_set1_ps(rayStart.x);
    __m512 rayStartY = _mm512_set1_ps(rayStart.y);
    __m512 rayStartZ = _mm512_set1_ps(rayStart.z);

    for (; i < simdEnd; i += 8) {
        __m512 v0x = _mm512_loadu_ps((float*)&v0[i].x);
        __m512 v0y = _mm512_loadu_ps((float*)&v0[i].y);
        __m512 v0z = _mm512_loadu_ps((float*)&v0[i].z);

        __m512 v1x = _mm512_loadu_ps((float*)&v1[i].x);
        __m512 v1y = _mm512_loadu_ps((float*)&v1[i].y);
        __m512 v1z = _mm512_loadu_ps((float*)&v1[i].z);

        __m512 v2x = _mm512_loadu_ps((float*)&v2[i].x);
        __m512 v2y = _mm512_loadu_ps((float*)&v2[i].y);
        __m512 v2z = _mm512_loadu_ps((float*)&v2[i].z);

        __m512 e1x = _mm512_sub_ps(v1x, v0x);
        __m512 e1y = _mm512_sub_ps(v1y, v0y);
        __m512 e1z = _mm512_sub_ps(v1z, v0z);

        __m512 e2x = _mm512_sub_ps(v2x, v0x);
        __m512 e2y = _mm512_sub_ps(v2y, v0y);
        __m512 e2z = _mm512_sub_ps(v2z, v0z);

        __m512 hx = _mm512_sub_ps(_mm512_mul_ps(diry, e2z), _mm512_mul_ps(dirz, e2y));
        __m512 hy = _mm512_sub_ps(_mm512_mul_ps(dirz, e2x), _mm512_mul_ps(dirx, e2z));
        __m512 hz = _mm512_sub_ps(_mm512_mul_ps(dirx, e2y), _mm512_mul_ps(diry, e2x));

        __m512 a = _mm512_add_ps(
            _mm512_add_ps(_mm512_mul_ps(e1x, hx), _mm512_mul_ps(e1y, hy)),
            _mm512_mul_ps(e1z, hz));

        __mmask16 maskParallel = _mm512_cmp_ps_mask(_mm512_abs_ps(a), _mm512_set1_ps(EPS), _CMP_LT_OS);

        if (maskParallel == 0xFFFF) continue;

        __m512 f = _mm512_div_ps(_mm512_set1_ps(1.0f), a);

        __m512 sx = _mm512_sub_ps(rayStartX, v0x);
        __m512 sy = _mm512_sub_ps(rayStartY, v0y);
        __m512 sz = _mm512_sub_ps(rayStartZ, v0z);

        __m512 u = _mm512_mul_ps(f, _mm512_add_ps(_mm512_add_ps(_mm512_mul_ps(sx, hx), _mm512_mul_ps(sy, hy)), _mm512_mul_ps(sz, hz)));

        __mmask16 maskU = _mm512_cmp_ps_mask(u, _mm512_set1_ps(0.0f), _CMP_GE_OS) &
            _mm512_cmp_ps_mask(u, _mm512_set1_ps(1.0f), _CMP_LE_OS);

        if (maskU == 0) continue;

        __m512 qx = _mm512_sub_ps(_mm512_mul_ps(sy, e1z), _mm512_mul_ps(sz, e1y));
        __m512 qy = _mm512_sub_ps(_mm512_mul_ps(sz, e1x), _mm512_mul_ps(sx, e1z));
        __m512 qz = _mm512_sub_ps(_mm512_mul_ps(sx, e1y), _mm512_mul_ps(sy, e1x));

        __m512 v = _mm512_mul_ps(f, _mm512_add_ps(_mm512_add_ps(_mm512_mul_ps(dirx, qx), _mm512_mul_ps(diry, qy)), _mm512_mul_ps(dirz, qz)));

        __mmask16 maskV = _mm512_cmp_ps_mask(v, _mm512_set1_ps(0.0f), _CMP_GE_OS) &
            _mm512_cmp_ps_mask(_mm512_add_ps(u, v), _mm512_set1_ps(1.0f), _CMP_LE_OS);

        if (maskV == 0) continue;

        __m512 t = _mm512_mul_ps(f, _mm512_add_ps(_mm512_add_ps(_mm512_mul_ps(e2x, qx), _mm512_mul_ps(e2y, qy)), _mm512_mul_ps(e2z, qz)));

        __mmask16 maskT = _mm512_cmp_ps_mask(t, _mm512_set1_ps(0.0f), _CMP_GE_OS) &
            _mm512_cmp_ps_mask(t, _mm512_set1_ps(1.0f), _CMP_LE_OS);

        if (maskT != 0) return true;
    }

    for (; i < n; ++i) {
        Vec3 e1 = { v1[i].x - v0[i].x, v1[i].y - v0[i].y, v1[i].z - v0[i].z };
        Vec3 e2 = { v2[i].x - v0[i].x, v2[i].y - v0[i].y, v2[i].z - v0[i].z };
        Vec3 h = cross(dir, e2);
        float a = dot(e1, h);
        if (std::fabs(a) < EPS) continue;
        float f = 1.0f / a;
        Vec3 s = { rayStart.x - v0[i].x, rayStart.y - v0[i].y, rayStart.z - v0[i].z };
        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f) continue;
        Vec3 q = cross(s, e1);
        float v = f * dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) continue;
        float t = f * dot(e2, q);
        if (t >= 0.0f && t <= 1.0f) return true;
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