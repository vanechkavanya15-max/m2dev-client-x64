#pragma once

#include "CPUIDFeatures.h"

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <string_view>

/**
 * @file SIMDFrustumCulling.h
 * @brief Modul SIMD Frustum Culling z 3 sciezkami wykonawczymi (AVX2+FMA3, SSE2, Scalar) dla x64 w C++23.
 *
 * Zapewnia wysokowydajne odrzucanie obiektow (culling) sfer i brył AABB wzgledem 6 plaszczyzn
 * bryly widzenia kamery (Frustum). Wyposazony w dynamiczny dispatcher bez branchingu w petli.
 */

namespace Client::Graphics
{
    /**
     * @brief Pojedyncza plaszczyzna w przestrzeni 3D: Ax + By + Cz + D = 0.
     * Kompatybilna binarnie z D3DXPLANE (4 kolejne liczby float).
     */
    struct Plane
    {
        float normalX{0.0f};
        float normalY{0.0f};
        float normalZ{0.0f};
        float d{0.0f};

        constexpr Plane() noexcept = default;
        constexpr Plane(float nx, float ny, float nz, float dist) noexcept
            : normalX(nx), normalY(ny), normalZ(nz), d(dist) {}
    };

    /**
     * @brief 6 plaszczyzn kamery (Left, Right, Top, Bottom, Near, Far).
     */
    struct FrustumPlanes
    {
        Plane planes[6];

        constexpr FrustumPlanes() noexcept = default;

        constexpr FrustumPlanes(const Plane inPlanes[6]) noexcept
        {
            for (size_t i = 0; i < 6; ++i)
                planes[i] = inPlanes[i];
        }

        void SetPlane(size_t index, float nx, float ny, float nz, float dVal) noexcept
        {
            if (index < 6)
            {
                planes[index] = Plane{nx, ny, nz, dVal};
            }
        }

        /**
         * @brief Normalizuje wektory normalne wszystkich 6 plaszczyzn do dlugosci 1.0f.
         */
        void Normalize() noexcept
        {
            for (auto& p : planes)
            {
                float lenSq = p.normalX * p.normalX + p.normalY * p.normalY + p.normalZ * p.normalZ;
                if (lenSq > 1e-12f)
                {
                    float invLen = 1.0f / std::sqrt(lenSq);
                    p.normalX *= invLen;
                    p.normalY *= invLen;
                    p.normalZ *= invLen;
                    p.d *= invLen;
                }
            }
        }

        /**
         * @brief Szablon pomocniczy do konwersji ze struktur D3DXPLANE (jesli uzywane z D3D9).
         */
        template <typename TD3DPlane>
        static FrustumPlanes FromD3DPlanes(const TD3DPlane* d3dPlanes) noexcept
        {
            FrustumPlanes result;
            for (size_t i = 0; i < 6; ++i)
            {
                result.planes[i].normalX = d3dPlanes[i].a;
                result.planes[i].normalY = d3dPlanes[i].b;
                result.planes[i].normalZ = d3dPlanes[i].c;
                result.planes[i].d       = d3dPlanes[i].d;
            }
            return result;
        }
    };

    /**
     * @brief Sfera ograniczajaca (Bounding Sphere).
     */
    struct BoundingSphere
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float radius{0.0f};

        constexpr BoundingSphere() noexcept = default;
        constexpr BoundingSphere(float inX, float inY, float inZ, float inRadius) noexcept
            : x(inX), y(inY), z(inZ), radius(inRadius) {}
    };

    /**
     * @brief Wyrownana do osi bryla ograniczajaca (Axis-Aligned Bounding Box).
     */
    struct BoundingAABB
    {
        float minX{0.0f};
        float minY{0.0f};
        float minZ{0.0f};
        float maxX{0.0f};
        float maxY{0.0f};
        float maxZ{0.0f};

        constexpr BoundingAABB() noexcept = default;
        constexpr BoundingAABB(float inMinX, float inMinY, float inMinZ,
                               float inMaxX, float inMaxY, float inMaxZ) noexcept
            : minX(inMinX), minY(inMinY), minZ(inMinZ),
              maxX(inMaxX), maxY(inMaxY), maxZ(inMaxZ) {}
    };

    /**
     * @brief Wynik testu widocznosci obiektu w bryle widzenia (Frustum).
     */
    enum class VisibilityResult : uint8_t
    {
        OUTSIDE = 0, ///< Obiekt calkowicie poza polem widzenia (odrzucony)
        INSIDE  = 1, ///< Obiekt calkowicie wewnatrz bryly widzenia
        PARTIAL = 2  ///< Obiekt przecina plaszczyzny bryly widzenia (czesciowo widoczny)
    };

    /**
     * @brief Dostepne sciezki wykonawcze (kernele) culling.
     */
    enum class KernelBackend : uint8_t
    {
        Auto   = 0, ///< Automatyczny wybor optymalnego kernela przez CPUID
        Scalar = 1, ///< Czysty C++ bez instrukcji wektorowych (fallback i referencja)
        SSE2   = 2, ///< Wektorowy SSE2 (4 obiekty na cykl)
        AVX2   = 3  ///< Wektorowy AVX2 + FMA3 (8 obiektow na cykl)
    };

    // Typy wskaznikow funkcji dla dispatchera wsadowego
    using CullSpheresFn = void (*)(const FrustumPlanes&, const BoundingSphere*, size_t, uint8_t*);
    using CullAABBFn    = void (*)(const FrustumPlanes&, const BoundingAABB*, size_t, uint8_t*);

    // =========================================================================
    // Dynamic Dispatcher API (wybiera optymalny kernel bez branchingu w petli)
    // =========================================================================

    /**
     * @brief Testuje tablice sfer z uzyciem optymalnego kernela CPU.
     * @param frustum 6 plaszczyzn kamery.
     * @param spheres Wskaznik na tablice BoundingSphere.
     * @param count Liczba sfer do przetestowania.
     * @param outVisibility Bufor wyjsciowy na wartosci VisibilityResult (jako uint8_t).
     */
    void FrustumCullSpheresBatch(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility);

    /**
     * @brief Testuje tablice brył AABB z uzyciem optymalnego kernela CPU.
     * @param frustum 6 plaszczyzn kamery.
     * @param aabbs Wskaznik na tablice BoundingAABB.
     * @param count Liczba AABB do przetestowania.
     * @param outVisibility Bufor wyjsciowy na wartosci VisibilityResult (jako uint8_t).
     */
    void FrustumCullAABBBatch(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility);

    // =========================================================================
    // Bezposredni dostep do poszczegolnych kerneli (Sciezki A, B, C)
    // =========================================================================

    // Sciezka A: AVX2 + FMA3 (8 sfer / 8 AABB w jednym cyklu)
    void FrustumCullSpheres_AVX2(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility);
    void FrustumCullAABB_AVX2(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility);

    // Sciezka B: SSE2 (4 sfery / 4 AABB w jednym cyklu)
    void FrustumCullSpheres_SSE2(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility);
    void FrustumCullAABB_SSE2(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility);

    // Sciezka C: Scalar (czysty C++ bez wektoryzacji - fallback)
    void FrustumCullSpheres_Scalar(const FrustumPlanes& frustum, const BoundingSphere* spheres, size_t count, uint8_t* outVisibility);
    void FrustumCullAABB_Scalar(const FrustumPlanes& frustum, const BoundingAABB* aabbs, size_t count, uint8_t* outVisibility);

    // =========================================================================
    // Pojedyncze testy pomocnicze
    // =========================================================================

    VisibilityResult FrustumCullSphere(const FrustumPlanes& frustum, const BoundingSphere& sphere);
    VisibilityResult FrustumCullAABB(const FrustumPlanes& frustum, const BoundingAABB& aabb);

    // =========================================================================
    // Zarzadzanie i diagnostyka dispatchera
    // =========================================================================

    void SetFrustumCullBackend(KernelBackend backend) noexcept;
    KernelBackend GetActiveFrustumCullBackend() noexcept;
    std::string_view GetActiveFrustumCullBackendName() noexcept;

} // namespace Client::Graphics
