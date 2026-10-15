#include "SIMDFrustumCulling.h"

#include <immintrin.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>

#if defined(__GNUC__) || defined(__clang__)
#define ATTRIBUTE_AVX2 __attribute__((target("avx2,fma")))
#else
#define ATTRIBUTE_AVX2
#endif

namespace Client::Graphics
{
    // =========================================================================
    // Tablica LUT (Lookup Table) do mapowania masek bitowych SIMD na VisibilityResult
    // Indeks: (is_outside << 1) | is_inside
    // =========================================================================
    static constexpr uint8_t kVisibilityLUT[4] = {
        static_cast<uint8_t>(VisibilityResult::PARTIAL), // 0: out=0, in=0 (przecina plaszczyzne)
        static_cast<uint8_t>(VisibilityResult::INSIDE),  // 1: out=0, in=1 (calkowicie w srodku)
        static_cast<uint8_t>(VisibilityResult::OUTSIDE), // 2: out=1, in=0 (poza bryla widzenia)
        static_cast<uint8_t>(VisibilityResult::OUTSIDE)  // 3: out=1, in=1 (teoretycznie niemozliwe, bezpieczny fallback)
    };

    // =========================================================================
    // SCIEZKA C: SCALAR (Bezpieczny fallback i wzorzec referencyjny)
    // =========================================================================

    void FrustumCullSpheres_Scalar(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility)
    {
        if (!spheres || !outVisibility || count == 0)
            return;

        for (size_t i = 0; i < count; ++i)
        {
            const auto& s = spheres[i];
            bool anyOutside = false;
            bool allInside = true;

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];
                const float dist = pl.normalX * s.x + pl.normalY * s.y + pl.normalZ * s.z + pl.d;

                // Odleglosc mniejsza niz -radius oznacza, ze cala sfera jest za plaszczyzna
                if (dist < -s.radius)
                {
                    anyOutside = true;
                    break;
                }

                // Odleglosc mniejsza niz +radius oznacza, ze sfera przecina plaszczyzne lub jest poza nia
                if (dist < s.radius)
                {
                    allInside = false;
                }
            }

            if (anyOutside)
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::OUTSIDE);
            else if (allInside)
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::INSIDE);
            else
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::PARTIAL);
        }
    }

    void FrustumCullAABB_Scalar(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility)
    {
        if (!aabbs || !outVisibility || count == 0)
            return;

        for (size_t i = 0; i < count; ++i)
        {
            const auto& b = aabbs[i];
            bool anyOutside = false;
            bool allInside = true;

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];

                // P-vertex: wierzcholek AABB najbardziej wysuniety w kierunku normalnej plaszczyzny
                const float px = (pl.normalX >= 0.0f) ? b.maxX : b.minX;
                const float py = (pl.normalY >= 0.0f) ? b.maxY : b.minY;
                const float pz = (pl.normalZ >= 0.0f) ? b.maxZ : b.minZ;

                const float distP = pl.normalX * px + pl.normalY * py + pl.normalZ * pz + pl.d;
                if (distP < 0.0f)
                {
                    anyOutside = true;
                    break;
                }

                // N-vertex: wierzcholek AABB najbardziej wysuniety w kierunku przeciwnym do normalnej
                const float nx = (pl.normalX >= 0.0f) ? b.minX : b.maxX;
                const float ny = (pl.normalY >= 0.0f) ? b.minY : b.maxY;
                const float nz = (pl.normalZ >= 0.0f) ? b.minZ : b.maxZ;

                const float distN = pl.normalX * nx + pl.normalY * ny + pl.normalZ * nz + pl.d;
                if (distN < 0.0f)
                {
                    allInside = false;
                }
            }

            if (anyOutside)
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::OUTSIDE);
            else if (allInside)
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::INSIDE);
            else
                outVisibility[i] = static_cast<uint8_t>(VisibilityResult::PARTIAL);
        }
    }

    // =========================================================================
    // SCIEZKA B: SSE2 (4 sfery lub 4 AABB w jednym cyklu)
    // Bezpieczne ladowanie unaligned (_mm_loadu_ps)
    // =========================================================================

    void FrustumCullSpheres_SSE2(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility)
    {
        if (!spheres || !outVisibility || count == 0)
            return;

        const size_t simdCount = count - (count % 4);

        for (size_t i = 0; i < simdCount; i += 4)
        {
            // Ladowanie 4 sfer (po 16 bajtow kazda) z pamieci przez unaligned load
            __m128 s0 = _mm_loadu_ps(reinterpret_cast<const float*>(spheres + i + 0));
            __m128 s1 = _mm_loadu_ps(reinterpret_cast<const float*>(spheres + i + 1));
            __m128 s2 = _mm_loadu_ps(reinterpret_cast<const float*>(spheres + i + 2));
            __m128 s3 = _mm_loadu_ps(reinterpret_cast<const float*>(spheres + i + 3));

            // Transpozycja 4x4: przejscie z ukladu AoS (Array of Structs) do SoA (Struct of Arrays)
            _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
            const __m128 vx = s0;
            const __m128 vy = s1;
            const __m128 vz = s2;
            const __m128 vr = s3;

            __m128 any_outside = _mm_setzero_ps();
            __m128 all_inside  = _mm_castsi128_ps(_mm_set1_epi32(-1));

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];
                const __m128 v_nx = _mm_set1_ps(pl.normalX);
                const __m128 v_ny = _mm_set1_ps(pl.normalY);
                const __m128 v_nz = _mm_set1_ps(pl.normalZ);
                const __m128 v_d  = _mm_set1_ps(pl.d);

                // dist = nx * vx + ny * vy + nz * vz + d
                __m128 dist = _mm_add_ps(_mm_mul_ps(v_nx, vx), v_d);
                dist = _mm_add_ps(dist, _mm_mul_ps(v_ny, vy));
                dist = _mm_add_ps(dist, _mm_mul_ps(v_nz, vz));

                // dist + radius < 0 => calkowicie poza plaszczyzna
                const __m128 dist_plus_r = _mm_add_ps(dist, vr);
                const __m128 is_out = _mm_cmplt_ps(dist_plus_r, _mm_setzero_ps());
                any_outside = _mm_or_ps(any_outside, is_out);

                // radius <= dist (odpowiednik dist >= radius) => calkowicie wewnatrz
                const __m128 is_in = _mm_cmple_ps(vr, dist);
                all_inside = _mm_and_ps(all_inside, is_in);
            }

            const int out_mask = _mm_movemask_ps(any_outside);
            const int in_mask  = _mm_movemask_ps(all_inside);

            for (int k = 0; k < 4; ++k)
            {
                const int o = (out_mask >> k) & 1;
                const int n = (in_mask  >> k) & 1;
                outVisibility[i + k] = kVisibilityLUT[(o << 1) | n];
            }
        }

        // Obsluga reszty elementow (tail) przez sciezke scalarna
        if (simdCount < count)
        {
            FrustumCullSpheres_Scalar(frustum, spheres + simdCount, count - simdCount, outVisibility + simdCount);
        }
    }

    void FrustumCullAABB_SSE2(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility)
    {
        if (!aabbs || !outVisibility || count == 0)
            return;

        const size_t simdCount = count - (count % 4);

        for (size_t i = 0; i < simdCount; i += 4)
        {
            alignas(16) float minX[4], minY[4], minZ[4], maxX[4], maxY[4], maxZ[4];
            for (int j = 0; j < 4; ++j)
            {
                const auto& b = aabbs[i + j];
                minX[j] = b.minX; minY[j] = b.minY; minZ[j] = b.minZ;
                maxX[j] = b.maxX; maxY[j] = b.maxY; maxZ[j] = b.maxZ;
            }

            const __m128 v_minX = _mm_loadu_ps(minX);
            const __m128 v_minY = _mm_loadu_ps(minY);
            const __m128 v_minZ = _mm_loadu_ps(minZ);
            const __m128 v_maxX = _mm_loadu_ps(maxX);
            const __m128 v_maxY = _mm_loadu_ps(maxY);
            const __m128 v_maxZ = _mm_loadu_ps(maxZ);

            __m128 any_outside = _mm_setzero_ps();
            __m128 all_inside  = _mm_castsi128_ps(_mm_set1_epi32(-1));

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];
                const __m128 v_nx = _mm_set1_ps(pl.normalX);
                const __m128 v_ny = _mm_set1_ps(pl.normalY);
                const __m128 v_nz = _mm_set1_ps(pl.normalZ);
                const __m128 v_d  = _mm_set1_ps(pl.d);

                // Wybor P-vertex i N-vertex wzgledem znaku wektora normalnego plaszczyzny
                const __m128 vpx = (pl.normalX >= 0.0f) ? v_maxX : v_minX;
                const __m128 vpy = (pl.normalY >= 0.0f) ? v_maxY : v_minY;
                const __m128 vpz = (pl.normalZ >= 0.0f) ? v_maxZ : v_minZ;

                const __m128 vnx = (pl.normalX >= 0.0f) ? v_minX : v_maxX;
                const __m128 vny = (pl.normalY >= 0.0f) ? v_minY : v_maxY;
                const __m128 vnz = (pl.normalZ >= 0.0f) ? v_minZ : v_maxZ;

                // dist_p = nx * px + ny * py + nz * pz + d
                __m128 dist_p = _mm_add_ps(_mm_mul_ps(v_nx, vpx), v_d);
                dist_p = _mm_add_ps(dist_p, _mm_mul_ps(v_ny, vpy));
                dist_p = _mm_add_ps(dist_p, _mm_mul_ps(v_nz, vpz));

                // dist_n = nx * nx_pt + ny * ny_pt + nz * nz_pt + d
                __m128 dist_n = _mm_add_ps(_mm_mul_ps(v_nx, vnx), v_d);
                dist_n = _mm_add_ps(dist_n, _mm_mul_ps(v_ny, vny));
                dist_n = _mm_add_ps(dist_n, _mm_mul_ps(v_nz, vnz));

                // dist_p < 0 => OUTSIDE
                const __m128 is_out = _mm_cmplt_ps(dist_p, _mm_setzero_ps());
                any_outside = _mm_or_ps(any_outside, is_out);

                // 0 <= dist_n (odpowiednik dist_n >= 0) => INSIDE
                const __m128 is_in = _mm_cmple_ps(_mm_setzero_ps(), dist_n);
                all_inside = _mm_and_ps(all_inside, is_in);
            }

            const int out_mask = _mm_movemask_ps(any_outside);
            const int in_mask  = _mm_movemask_ps(all_inside);

            for (int k = 0; k < 4; ++k)
            {
                const int o = (out_mask >> k) & 1;
                const int n = (in_mask  >> k) & 1;
                outVisibility[i + k] = kVisibilityLUT[(o << 1) | n];
            }
        }

        if (simdCount < count)
        {
            FrustumCullAABB_Scalar(frustum, aabbs + simdCount, count - simdCount, outVisibility + simdCount);
        }
    }

    // =========================================================================
    // SCIEZKA A: AVX2 + FMA3 (8 sfer lub 8 AABB w jednym cyklu)
    // Bezpieczne ladowanie unaligned (_mm256_loadu_ps)
    // =========================================================================

    ATTRIBUTE_AVX2
    void FrustumCullSpheres_AVX2(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility)
    {
        if (!spheres || !outVisibility || count == 0)
            return;

        const size_t simdCount = count - (count % 8);

        for (size_t i = 0; i < simdCount; i += 8)
        {
            // Ladowanie 8 sfer (128 bajtow) za pomoca unaligned load _mm256_loadu_ps
            const __m256 r0 = _mm256_loadu_ps(reinterpret_cast<const float*>(spheres + i + 0));
            const __m256 r1 = _mm256_loadu_ps(reinterpret_cast<const float*>(spheres + i + 2));
            const __m256 r2 = _mm256_loadu_ps(reinterpret_cast<const float*>(spheres + i + 4));
            const __m256 r3 = _mm256_loadu_ps(reinterpret_cast<const float*>(spheres + i + 6));

            __m128 s0 = _mm256_castps256_ps128(r0);
            __m128 s1 = _mm256_extractf128_ps(r0, 1);
            __m128 s2 = _mm256_castps256_ps128(r1);
            __m128 s3 = _mm256_extractf128_ps(r1, 1);
            __m128 s4 = _mm256_castps256_ps128(r2);
            __m128 s5 = _mm256_extractf128_ps(r2, 1);
            __m128 s6 = _mm256_castps256_ps128(r3);
            __m128 s7 = _mm256_extractf128_ps(r3, 1);

            _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
            _MM_TRANSPOSE4_PS(s4, s5, s6, s7);

            const __m256 vx = _mm256_set_m128(s4, s0);
            const __m256 vy = _mm256_set_m128(s5, s1);
            const __m256 vz = _mm256_set_m128(s6, s2);
            const __m256 vr = _mm256_set_m128(s7, s3);

            __m256 any_outside = _mm256_setzero_ps();
            __m256 all_inside  = _mm256_castsi256_ps(_mm256_set1_epi32(-1));

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];
                const __m256 v_nx = _mm256_set1_ps(pl.normalX);
                const __m256 v_ny = _mm256_set1_ps(pl.normalY);
                const __m256 v_nz = _mm256_set1_ps(pl.normalZ);
                const __m256 v_d  = _mm256_set1_ps(pl.d);

                // Obliczanie odleglosci za pomoca potrojnej fuzji FMA3: nx*vx + ny*vy + nz*vz + d
                __m256 dist = _mm256_fmadd_ps(v_nx, vx, v_d);
                dist = _mm256_fmadd_ps(v_ny, vy, dist);
                dist = _mm256_fmadd_ps(v_nz, vz, dist);

                // dist + vr < 0 => OUTSIDE
                const __m256 dist_plus_r = _mm256_add_ps(dist, vr);
                const __m256 is_out = _mm256_cmp_ps(dist_plus_r, _mm256_setzero_ps(), _CMP_LT_OQ);
                any_outside = _mm256_or_ps(any_outside, is_out);

                // dist - vr >= 0 => INSIDE
                const __m256 dist_minus_r = _mm256_sub_ps(dist, vr);
                const __m256 is_in = _mm256_cmp_ps(dist_minus_r, _mm256_setzero_ps(), _CMP_GE_OQ);
                all_inside = _mm256_and_ps(all_inside, is_in);
            }

            const int out_mask = _mm256_movemask_ps(any_outside);
            const int in_mask  = _mm256_movemask_ps(all_inside);

            for (int k = 0; k < 8; ++k)
            {
                const int o = (out_mask >> k) & 1;
                const int n = (in_mask  >> k) & 1;
                outVisibility[i + k] = kVisibilityLUT[(o << 1) | n];
            }
        }

        if (simdCount < count)
        {
            FrustumCullSpheres_Scalar(frustum, spheres + simdCount, count - simdCount, outVisibility + simdCount);
        }
    }

    ATTRIBUTE_AVX2
    void FrustumCullAABB_AVX2(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility)
    {
        if (!aabbs || !outVisibility || count == 0)
            return;

        const size_t simdCount = count - (count % 8);

        for (size_t i = 0; i < simdCount; i += 8)
        {
            alignas(32) float minX[8], minY[8], minZ[8], maxX[8], maxY[8], maxZ[8];
            for (int j = 0; j < 8; ++j)
            {
                const auto& b = aabbs[i + j];
                minX[j] = b.minX; minY[j] = b.minY; minZ[j] = b.minZ;
                maxX[j] = b.maxX; maxY[j] = b.maxY; maxZ[j] = b.maxZ;
            }

            const __m256 v_minX = _mm256_loadu_ps(minX);
            const __m256 v_minY = _mm256_loadu_ps(minY);
            const __m256 v_minZ = _mm256_loadu_ps(minZ);
            const __m256 v_maxX = _mm256_loadu_ps(maxX);
            const __m256 v_maxY = _mm256_loadu_ps(maxY);
            const __m256 v_maxZ = _mm256_loadu_ps(maxZ);

            __m256 any_outside = _mm256_setzero_ps();
            __m256 all_inside  = _mm256_castsi256_ps(_mm256_set1_epi32(-1));

            for (size_t p = 0; p < 6; ++p)
            {
                const auto& pl = frustum.planes[p];
                const __m256 v_nx = _mm256_set1_ps(pl.normalX);
                const __m256 v_ny = _mm256_set1_ps(pl.normalY);
                const __m256 v_nz = _mm256_set1_ps(pl.normalZ);
                const __m256 v_d  = _mm256_set1_ps(pl.d);

                const __m256 vpx = (pl.normalX >= 0.0f) ? v_maxX : v_minX;
                const __m256 vpy = (pl.normalY >= 0.0f) ? v_maxY : v_minY;
                const __m256 vpz = (pl.normalZ >= 0.0f) ? v_maxZ : v_minZ;

                const __m256 vnx = (pl.normalX >= 0.0f) ? v_minX : v_maxX;
                const __m256 vny = (pl.normalY >= 0.0f) ? v_minY : v_maxY;
                const __m256 vnz = (pl.normalZ >= 0.0f) ? v_minZ : v_maxZ;

                // FMA3 dla P-vertex
                __m256 dist_p = _mm256_fmadd_ps(v_nx, vpx, v_d);
                dist_p = _mm256_fmadd_ps(v_ny, vpy, dist_p);
                dist_p = _mm256_fmadd_ps(v_nz, vpz, dist_p);

                // FMA3 dla N-vertex
                __m256 dist_n = _mm256_fmadd_ps(v_nx, vnx, v_d);
                dist_n = _mm256_fmadd_ps(v_ny, vny, dist_n);
                dist_n = _mm256_fmadd_ps(v_nz, vnz, dist_n);

                // dist_p < 0 => OUTSIDE
                const __m256 is_out = _mm256_cmp_ps(dist_p, _mm256_setzero_ps(), _CMP_LT_OQ);
                any_outside = _mm256_or_ps(any_outside, is_out);

                // dist_n >= 0 => INSIDE
                const __m256 is_in = _mm256_cmp_ps(dist_n, _mm256_setzero_ps(), _CMP_GE_OQ);
                all_inside = _mm256_and_ps(all_inside, is_in);
            }

            const int out_mask = _mm256_movemask_ps(any_outside);
            const int in_mask  = _mm256_movemask_ps(all_inside);

            for (int k = 0; k < 8; ++k)
            {
                const int o = (out_mask >> k) & 1;
                const int n = (in_mask  >> k) & 1;
                outVisibility[i + k] = kVisibilityLUT[(o << 1) | n];
            }
        }

        if (simdCount < count)
        {
            FrustumCullAABB_Scalar(frustum, aabbs + simdCount, count - simdCount, outVisibility + simdCount);
        }
    }

    // =========================================================================
    // Dynamic Dispatcher (Inicjalizacja wskaznikow raz, zero branchingu w petli)
    // =========================================================================

    static std::atomic<CullSpheresFn> s_activeCullSpheresFn{nullptr};
    static std::atomic<CullAABBFn>    s_activeCullAABBFn{nullptr};
    static std::atomic<KernelBackend> s_activeBackend{KernelBackend::Auto};

    static void ResolveDispatchers() noexcept
    {
        const auto& cpu = CPUIDFeatures::Get();
        if (cpu.HasAVX2() && cpu.HasFMA3())
        {
            s_activeCullSpheresFn.store(&FrustumCullSpheres_AVX2, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_AVX2, std::memory_order_release);
            s_activeBackend.store(KernelBackend::AVX2, std::memory_order_release);
        }
        else if (cpu.HasSSE2())
        {
            s_activeCullSpheresFn.store(&FrustumCullSpheres_SSE2, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_SSE2, std::memory_order_release);
            s_activeBackend.store(KernelBackend::SSE2, std::memory_order_release);
        }
        else
        {
            s_activeCullSpheresFn.store(&FrustumCullSpheres_Scalar, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_Scalar, std::memory_order_release);
            s_activeBackend.store(KernelBackend::Scalar, std::memory_order_release);
        }
    }

    void FrustumCullSpheresBatch(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility)
    {
        if (!spheres || !outVisibility || count == 0)
            return;

        auto fn = s_activeCullSpheresFn.load(std::memory_order_relaxed);
        if (!fn) [[unlikely]]
        {
            ResolveDispatchers();
            fn = s_activeCullSpheresFn.load(std::memory_order_relaxed);
        }
        fn(frustum, spheres, count, outVisibility);
    }

    void FrustumCullAABBBatch(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility)
    {
        if (!aabbs || !outVisibility || count == 0)
            return;

        auto fn = s_activeCullAABBFn.load(std::memory_order_relaxed);
        if (!fn) [[unlikely]]
        {
            ResolveDispatchers();
            fn = s_activeCullAABBFn.load(std::memory_order_relaxed);
        }
        fn(frustum, aabbs, count, outVisibility);
    }

    VisibilityResult FrustumCullSphere(const FrustumPlanes& frustum, const BoundingSphere& sphere)
    {
        uint8_t res = 0;
        FrustumCullSpheresBatch(frustum, &sphere, 1, &res);
        return static_cast<VisibilityResult>(res);
    }

    VisibilityResult FrustumCullAABB(const FrustumPlanes& frustum, const BoundingAABB& aabb)
    {
        uint8_t res = 0;
        FrustumCullAABBBatch(frustum, &aabb, 1, &res);
        return static_cast<VisibilityResult>(res);
    }

    void SetFrustumCullBackend(KernelBackend backend) noexcept
    {
        switch (backend)
        {
        case KernelBackend::Scalar:
            s_activeCullSpheresFn.store(&FrustumCullSpheres_Scalar, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_Scalar, std::memory_order_release);
            s_activeBackend.store(KernelBackend::Scalar, std::memory_order_release);
            break;
        case KernelBackend::SSE2:
            s_activeCullSpheresFn.store(&FrustumCullSpheres_SSE2, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_SSE2, std::memory_order_release);
            s_activeBackend.store(KernelBackend::SSE2, std::memory_order_release);
            break;
        case KernelBackend::AVX2:
            s_activeCullSpheresFn.store(&FrustumCullSpheres_AVX2, std::memory_order_release);
            s_activeCullAABBFn.store(&FrustumCullAABB_AVX2, std::memory_order_release);
            s_activeBackend.store(KernelBackend::AVX2, std::memory_order_release);
            break;
        case KernelBackend::Auto:
        default:
            ResolveDispatchers();
            break;
        }
    }

    KernelBackend GetActiveFrustumCullBackend() noexcept
    {
        auto b = s_activeBackend.load(std::memory_order_relaxed);
        if (b == KernelBackend::Auto)
        {
            ResolveDispatchers();
            b = s_activeBackend.load(std::memory_order_relaxed);
        }
        return b;
    }

    std::string_view GetActiveFrustumCullBackendName() noexcept
    {
        switch (GetActiveFrustumCullBackend())
        {
        case KernelBackend::AVX2:   return "AVX2+FMA3";
        case KernelBackend::SSE2:   return "SSE2";
        case KernelBackend::Scalar: return "Scalar";
        default:                    return "Unknown";
        }
    }

} // namespace Client::Graphics
